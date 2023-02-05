#ifndef FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_HPP
#define FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_HPP

#include "FastCaptureDef.h"
#include <optional>
#include <atomic>
#include <string>
#include <Windows.h>
#include <processthreadsapi.h>
#include "DllData.hpp"
#include "../FastCaptureInjectDllDef.h"
#include "../../Utils/GLUtils.hpp"
#include "../../Utils/Windows/UtilsWindows.hpp"

FAST_CAPTURE_NAMESPACE
{
    class WglContext
    {
    private:
        Windows::UniqueHDC h_gl_dc_{};
        Windows::UniqueGLRC h_gl_rc_{};
        std::optional<GLuint> opt_shared_texture_id_{};
        UniqueOpenGLFbo opt_fbo_id_{};
        FastCaptureFullErrorCode error_code1_ = FastCaptureMakeSuccessValue();

      public:
        WglContext() = default;
        ~WglContext() = default;

        FastCaptureErrorCode ShareContextFrom(HGLRC target)
        {
            if (h_gl_dc_.IsInvalid())
            {
                h_gl_dc_ = ::CreateCompatibleDC(nullptr);
            }
            Utils::Destroy(h_gl_rc_);
            Utils::Emplace(h_gl_rc_, ::wglCreateContext(h_gl_dc_.Get()));

            error_code1_ = FastCaptureMakeSuccessValue();
            return error_code1_.error_code;
        }
    };

    class ReadPixelsThread
    {
    public:
        enum class Command
        {
            ReadPixels,
            Exit
        };

    private:
        /**
         * @brief 不确定GCC的std::thread是否支持，在DLL中创建线程时，
            会自动增加DLL的引用计数的功能，因此使用系统API实现
         *
         */
        Windows::UniqueHandleInvalidNULL h_read_pixel_thread_{nullptr};
        Windows::UniqueHandleInvalidNULL h_is_thread_init_finish_{nullptr};
        /**
         * @brief 这是一个AUTO_RESET的，默认值为0的信号量。
            当此信号量变为1时，线程调用的::WaitForSingleObject返回，
            通知线程开始执行::glReadPixel函数
         *
         */
        Windows::UniqueHandleInvalidNULL h_is_need_run_command_{nullptr};
        DWORD win32_last_error_{0};
        GLenum glew_init_error_code_{0};
        /**
         * @brief 在DllMain中收到退出信息后，会clear这个值
         *
         */
        Command command_{Command::ReadPixels};
        std::size_t last_capture_image_size_{0};
        /**
        * @brief 在DllMain函数中收到卸载的信息后，调用RequestThreadStop会使此值变为false。
           创建线程后应当立即检查这个值，若它为false，则表明线程创建失败。
        *
        */
        std::atomic_flag is_available = ATOMIC_FLAG_INIT;
        bool is_shared_gl_rc_changed_{false};

        static std::size_t GetCaptureImageSize() noexcept
        {
            auto& dll_data = DllData::GetInstance();
            std::lock_guard<std::mutex> capture_descriptor_lock_guard{
                dll_data.p_capture_data_.Get()->lock};
            std::size_t result =
                dll_data.p_capture_descriptor_.Get()->GetHeight() *
                dll_data.p_capture_descriptor_.Get()->GetWidth();
            return result;
        }

        FastCaptureErrorCode PrepareCaptureImage(const std::wstring_view capture_image_name) noexcept
        {
            auto image_size = GetCaptureImageSize();
            if (image_size > last_capture_image_size_)
            {
                DWORD dword_image_size[2];
                Utils::MemSet(&dword_image_size);
                static_assert(sizeof(dword_image_size) == sizeof(std::size_t),
                              "Size of DWORD[2] must be equal to size of std::size_t.");
                Utils::CopyData(&image_size, dword_image_size);

                Windows::UniqueHandleInvalidNULL h_capture_image =
                    ::CreateFileMappingW(
                        INVALID_HANDLE_VALUE,
                        NULL,
                        PAGE_READWRITE,
                        dword_image_size[1],
                        dword_image_size[0],
                        capture_image_name.data());
                if (h_capture_image.IsInvalid())
                {
                    return FAST_CAPTURE_E_READ_PIXELS_THREAD_CREATE_SHARED_CAPTURE_IMAGE_FAILED;
                }
                Windows::UniqueMapViewOfFile<CaptureImage> p_capture_image =
                    reinterpret_cast<CaptureImage*>(
                        ::MapViewOfFile(
                            h_capture_image.Get(),
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            image_size));
                if (p_capture_image.IsInvalid())
                {
                    return FAST_CAPTURE_E_READ_PIXELS_THREAD_CREATE_SHARED_CAPTURE_IMAGE_MAP_OF_VIEW_FAILED;
                }
                auto& dll_data = DllData::GetInstance();
                {
                    std::lock_guard<std::mutex> capture_descriptor_lock_guard{
                        dll_data.p_capture_descriptor_.Get()->lock};
                    dll_data.p_capture_descriptor_.Get()->p_capture_image = p_capture_image.Get();
                }
                dll_data.h_capture_image_ = std::move(h_capture_image);
                dll_data.p_capture_data_ = std::move(p_capture_image);
                last_capture_image_size_ = image_size;
            }
            return FAST_CAPTURE_S_OK;
        }

        static FastCaptureErrorCode DoReadPixelsThreadCommand(ReadPixelsThread* p_thread_data)
        {
            auto capture_image_shared_name =
                std::wstring(L"Global\\") + DllData::GetInstance().shared_memory_name_prefix_;

            while (p_thread_data->is_available.test())
            {
                ::WaitForSingleObject(
                    p_thread_data->h_is_need_run_command_.Get(),
                    INFINITE);
                switch (p_thread_data->command_)
                {
                case Command::ReadPixels:
                {
                    auto prepare_shared_capture_image_memory_result =
                        p_thread_data->PrepareCaptureImage(capture_image_shared_name);
                    if (prepare_shared_capture_image_memory_result)
                    {
                        return prepare_shared_capture_image_memory_result;
                    }
                    if (p_thread_data->is_shared_gl_rc_changed_)
                    {
                        Utils::Destroy(p_thread_data->h_gl_rc_);

                        p_thread_data->is_shared_gl_rc_changed_ = false;
                    }
                    continue;
                }
                default:
                    return FAST_CAPTURE_S_OK;
                }
            }
            return FAST_CAPTURE_E_READ_PIXELS_THREAD_NOT_AVAILABLE;
        }

        static DWORD WINAPI Do(void* p_data)
        {
            auto p_thread_data =
                reinterpret_cast<
                    std::add_pointer_t<
                        ReadPixelsThread>>(p_data);

            /**
             * @brief GetModuleHandleExW成功后，不要使用return，而是使用此函数
             *
             */
            const auto exit_thread =
                [](FastCaptureErrorCode exit_code, HMODULE h_current_dll)
            { ::FreeLibraryAndExitThread(h_current_dll, exit_code); };
            /**
             * @brief 初始化结束后调用此函数
             *
             */
            const auto notify_init_finish = [&p_thread_data]()
            { ::SetEvent(p_thread_data->h_is_thread_init_finish_.Get()); };

            HMODULE h_current_dll;
            auto get_self_h_dll_result = ::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                L"FastCaptureInitInjectDll",
                &h_current_dll);
            if (get_self_h_dll_result)
            {
                p_thread_data->h_gl_dc_ = ::CreateCompatibleDC(nullptr);
                if (p_thread_data->h_gl_dc_.IsInvalid())
                {
                    p_thread_data->win32_last_error_ = ::GetLastError();
                    notify_init_finish();
                    exit_thread(
                        FAST_CAPTURE_E_READ_PIXELS_THREAD_CREATE_DC_FAILED,
                        h_current_dll);
                }
                p_thread_data->h_gl_rc_ = ::wglCreateContext(p_thread_data->h_gl_dc_.Get());
                if (p_thread_data->h_gl_rc_.IsInvalid())
                {
                    p_thread_data->win32_last_error_ = ::GetLastError();
                    notify_init_finish();
                    exit_thread(
                        FAST_CAPTURE_E_READ_PIXELS_THREAD_CREATE_GL_RC_FAILED,
                        h_current_dll);
                }
                if (!::wglMakeCurrent(p_thread_data->h_gl_dc_.Get(), p_thread_data->h_gl_rc_.Get()))
                {
                    p_thread_data->win32_last_error_ = ::GetLastError();
                    notify_init_finish();
                    exit_thread(
                        FAST_CAPTURE_E_READ_PIXELS_THREAD_MAKE_GL_CURRENT_FAILED,
                        h_current_dll);
                }
                p_thread_data->glew_init_error_code_ = ::glewInit();
                if (p_thread_data->glew_init_error_code_ != GLEW_OK)
                {
                    notify_init_finish();
                    exit_thread(
                        FAST_CAPTURE_E_READ_PIXELS_THREAD_INIT_GLEW_FAILED,
                        h_current_dll);
                }

                p_thread_data->is_available.test_and_set();
                notify_init_finish();
                auto do_read_pixels_result = DoReadPixelsThreadCommand(p_thread_data);
                exit_thread(do_read_pixels_result, h_current_dll);
            }

            p_thread_data->win32_last_error_ = ::GetLastError();
            notify_init_finish();
            return FAST_CAPTURE_E_READ_PIXELS_THREAD_GET_SELF_DLL_HANDLE_FAILED;
        }

        ReadPixelsThread() = default;
        ~ReadPixelsThread() = default;

    public:
        ReadPixelsThread(const ReadPixelsThread&) = delete;
        ReadPixelsThread& operator=(const ReadPixelsThread&) = delete;
        FastCaptureErrorCode Reinitialize()
        {
            Utils::Destroy(*this);
            Utils::Emplace(*this);

            h_is_need_run_command_ = ::CreateEvent(
                nullptr,
                TRUE,
                FALSE,
                nullptr);
            h_is_thread_init_finish_ = ::CreateEvent(
                nullptr,
                TRUE,
                FALSE,
                nullptr);
            h_read_pixel_thread_ = ::CreateThread(
                nullptr,
                0,
                &Do,
                this,
                0,
                nullptr);
            auto wait_read_pixel_thread_init_result = ::WaitForSingleObject(
                h_is_thread_init_finish_.Get(),
                4000);
            switch (wait_read_pixel_thread_init_result)
            {
            case WAIT_TIMEOUT:
                return FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_TIMEOUT;
            case WAIT_FAILED:
                return FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_FAILED;
            default:
                throw Windows::WaitResultNotExpectedException{wait_read_pixel_thread_init_result};
            }
            return FAST_CAPTURE_S_OK;
        }

        void SetSharedTextureId(GLuint shared_texture_id)
        {
            opt_shared_texture_id_ = shared_texture_id;
            if (opt_fbo_id_)
            {
                ::glDeleteFramebuffers(1, &opt_fbo_id_.value());
            }
            GLuint fbo_id;
            ::glCreateFramebuffers(1, &fbo_id);

            opt_fbo_id_ = {fbo_id};
        }

        void SetSharedGLRC(HGLRC h_emulator_gl_rc) noexcept
        {
            h_emulator_gl_rc_ = h_emulator_gl_rc;
            is_shared_gl_rc_changed_ = true;
        }

        void RequestThreadStop()
        {
            command_ = Command::Exit;
            ::SetEvent(h_is_need_run_command_.Get());
        }

        DWORD GetWin32ErrorCode() const noexcept
        {
            return win32_last_error_;
        }

        GLenum GetGlewInitError() const noexcept
        {
            return glew_init_error_code_;
        }

        static ReadPixelsThread& GetInstance() noexcept
        {
            static ReadPixelsThread result{};
            return result;
        };
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_HPP
