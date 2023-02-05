#include "WglContext.h"
#include "../../Utils/GLUtils.hpp"

FAST_CAPTURE_NAMESPACE
{
    namespace Details
    {
        WNDCLASSA RegisterWindowClass(const WNDCLASSA& target_class)
        {
            if (!::RegisterClassA(&target_class))
            {
                throw Windows::MakeError(FAST_CAPTURE_E_CREATE_GL_BASIC_WINDOW_CLASS_FAILED);
            }
            return target_class;
        }

        using UniqueWindowClassABase = Utils::RAIIWrapper<WNDCLASSA, decltype([](const WNDCLASSA& window_class)
                                                                              { ::UnregisterClassA(window_class.lpszClassName, window_class.hInstance); })>;
    }

    class UniqueWindowClassA : public Details::UniqueWindowClassABase
    {
    public:
        using Base = Details::UniqueWindowClassABase;
        using Base::Base;

        const WNDCLASSA& GetRef() const noexcept
        {
            return Base::GetRef();
        }
    };

    LRESULT CALLBACK GlBasicWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        switch (message)
        {
        case WM_PAINT: // 不需要绘制，所以敷衍一下系统
        {
            PAINTSTRUCT ps;
            /* HDC hdc */ std::ignore = ::BeginPaint(hwnd, &ps);
            ::EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        return 0;
    }

    const WNDCLASSA& GlBasicWindow::RegisterWindowClassIfNecessary()
    {
        static UniqueWindowClassA instance = Details::RegisterWindowClass(
            WNDCLASSA{
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = &WindowProc,
                .cbClsExtra = 0,
                .cbWndExtra = 0,
                .hInstance = GetHInstance(),
                .hIcon = nullptr,
                .hCursor = ::LoadCursor(nullptr, IDC_ARROW),
                .hbrBackground = nullptr,
                .lpszMenuName = nullptr,
                .lpszClassName = "GLBasicWindow{B2E83CEE-05E4-126F-49E0-15057BE889B1}",
            });
        return instance.GetRef();
    }

    HINSTANCE GlBasicWindow::GetHInstance()
    {
        auto result = ::GetModuleHandleA(nullptr);
        if (!result)
        {
            throw Windows::MakeError(FAST_CAPTURE_E_GET_EXE_INSTANCE_FAILED);
        }
        return result;
    }

    DWORD WINAPI GlBasicWindow::UiThreadFunction(LPVOID lpThreadParameter)
    {
        auto const p_this = reinterpret_cast<GlBasicWindow*>(lpThreadParameter);
        auto& error_code = p_this->ui_thread_error_code_;

        Windows::FreeLibraryAndExitThreadGuard free_library_and_exit_thread_guard{nullptr};
        {
            Utils::RAIIWrapper<HANDLE, decltype([](HANDLE h_is_init_finish)
                                                { ::SetEvent(h_is_init_finish); })>
                notify_init_finish_guard{p_this->h_is_thread_init_finish_.Get()};

            HMODULE h_current_dll;
            error_code = Windows::GetCurrentModuleAndAddCurrentModuleRefCount(&h_current_dll);
            if (!Utils::IsOk(error_code))
            {
                return error_code.error_code;
            }
            free_library_and_exit_thread_guard = h_current_dll;

            error_code = p_this->InitializeForThread();
            if (!Utils::IsOk(error_code))
            {
                return error_code.error_code;
            }
        }

        MSG msg;
        auto hwnd = p_this->hwnd_.Get();
        while (
            ::PeekMessageW(
                &msg,
                hwnd,
                0,
                0,
                PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        error_code = FastCaptureMakeSuccessValue();
        return error_code.error_code;
    }

    FastCaptureErrorCode GlBasicWindow::InitializeForThread()
    {
        const WNDCLASSA* p_window_class;
        try
        {
            p_window_class = &RegisterWindowClassIfNecessary();
        }
        catch (const FastCaptureErrorCode error_code)
        {
            return error_code;
        }
        hwnd_ = ::CreateWindowExA(
            0,
            p_window_class->lpszClassName,
            window_name_.c_str(),
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            p_window_class->hInstance,
            nullptr);
        if (hwnd_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_CREATE_GL_BASIC_WINDOW_FAILED);
        }

        hdc_ = ::GetDC(hwnd_.Get());
        if (hdc_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_GET_GL_BASIC_WINDOW_DC_FAILED);
        }

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        int pixel_format = ::ChoosePixelFormat(hdc_.Get(), &pfd);
        if (!pixel_format)
        {
            return Windows::MakeError(FAST_CAPTURE_E_CHOOSE_PIXEL_FORMAT_FAILED);
        }
        if (!::SetPixelFormat(hdc_.Get(), pixel_format, &pfd))
        {
            return Windows::MakeError(FAST_CAPTURE_E_SET_PIXEL_FORMAT);
        }

        return FastCaptureMakeSuccessValue();
    }

    GlBasicWindow::GlBasicWindow(const std::string& window_name)
        : window_name_(window_name)
    {
    }

    FastCaptureErrorCode GlBasicWindow::Initialize(const std::uint32_t timeout_ms)
    {
        h_is_thread_init_finish_ = ::CreateEvent(
            nullptr,
            TRUE,
            FALSE,
            nullptr);
        if (h_is_thread_init_finish_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_CREATE_THREAD_INIT_EVENT_FAILED);
        }

        h_ui_thread_ = ::CreateThread(
            nullptr,
            0,
            &UiThreadFunction,
            this,
            0,
            nullptr);
        if (h_ui_thread_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_CREATE_INIT_THREAD_FAILED);
        }

        auto wait_result = ::WaitForSingleObject(
            h_ui_thread_.Get(),
            timeout_ms);
        switch (wait_result)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_WAIT_THREAD_INIT_TIMEOUT);
        case WAIT_FAILED:
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_WAIT_THREAD_INIT_FAILED);
        default:
            return Windows::MakeError(FAST_CAPTURE_E_WAIT_RESULT_UNEXPECTED);
        }

        return ui_thread_error_code_;
    }

    HDC GlBasicWindow::GetHdc() const noexcept
    {
        return hdc_.Get();
    }

    FastCaptureErrorCode WglContext::Initialize(const std::uint32_t timeout_ms)
    {
        auto result = gl_window_.Initialize(timeout_ms);
        if (!Utils::IsOk(result))
        {
            return result;
        }
        return FastCaptureMakeSuccessValue();
    }

    FastCaptureErrorCode WglContext::RecreateGlRcAndShareContextFrom(HGLRC gl_rc)
    {
        auto hdc = gl_window_.GetHdc();

        gl_rc_ = ::wglCreateContext(hdc);
        if (gl_rc_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_WGL_CREATE_CONTEXT_FAILED);
        }

        if (!::wglMakeCurrent(hdc, gl_rc_.Get()))
        {
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_WGL_MAKE_CURRENT_FAILED);
        }

        auto glew_init_result = ::glewInit();
        if (glew_init_result != GLEW_OK)
        {
            return {
                FAST_CAPTURE_E_GL_BASIC_WINDOW_GLEW_INIT_FAILED,
                FAST_CAPTURE_ERROR_TYPE_GLEW,
                glew_init_result};
        }

        if (!::wglShareLists(gl_rc_.Get(), gl_rc))
        {
            return Windows::MakeError(FAST_CAPTURE_E_GL_BASIC_WINDOW_WGL_SHARE_LISTS_FAILED);
        }

        return FastCaptureMakeSuccessValue();
    }
}
