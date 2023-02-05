#ifndef FAST_CAPTURE_INJECT_DLL_WGL_CONTEXT_H
#define FAST_CAPTURE_INJECT_DLL_WGL_CONTEXT_H

#include "FastCaptureDef.h"
#include <windows.h>
#include <string>
#include "GL/glew.h"
#include "GL/gl.h"
#include "../../Utils/Windows/UtilsWindows.hpp"

// https://stackoverflow.com/questions/45518843/initializing-opengl-without-libraries
// https://stackoverflow.com/questions/30015597/how-can-i-create-opengl-context-using-windows-memory-dc-c

FAST_CAPTURE_NAMESPACE
{
    class GlBasicWindow
    {
    private:
        std::string window_name_;
        Windows::UniqueHwnd hwnd_{nullptr};
        Windows::UniqueHdc hdc_{nullptr};
        Windows::UniqueHandleInvalidNULL h_ui_thread_{nullptr};
        /**
        * @brief 这是一个AUTO_RESET的，默认值为0的信号量。
           当此信号量变为1时，外部调用的::WaitForSingleObject返回，
           表示线程初始化完成
        *
        */
        Windows::UniqueHandleInvalidNULL h_is_thread_init_finish_{nullptr};
        FastCaptureErrorCode ui_thread_error_code_{FastCaptureMakeSuccessValue()};

        static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM) noexcept;
        /**
         * @brief 若Window Class未注册，则尝试注册，失败则抛出FastCaptureErrorCode异常
         *
         * @return const WNDCLASSA& 成功则返回注册时所用的WNDCLASS
         */
        static const WNDCLASSA& RegisterWindowClassIfNecessary();
        /**
         * @brief 获得当前程序的HInstance，失败则抛出FastCaptureErrorCode异常

         *
         * @return HINSTANCE
         */
        static HINSTANCE GetHInstance();

        static DWORD WINAPI UiThreadFunction(LPVOID lpThreadParameter);

        FastCaptureErrorCode InitializeForThread();

    public:
        GlBasicWindow(const std::string& window_name);
        ~GlBasicWindow() = default;
        GlBasicWindow(const GlBasicWindow&) = delete;
        GlBasicWindow& operator=(const GlBasicWindow&) = delete;

        FastCaptureErrorCode Initialize(const std::uint32_t timeout_ms);
        HDC GetHdc() const noexcept;
    };

    class WglContext
    {
    private:
        GlBasicWindow gl_window_;
        Windows::UniqueGlRc gl_rc_{nullptr};
        Windows::UniqueHandleInvalidNULL h_window_process_thread_{nullptr};

    public:
        WglContext();
        ~WglContext();

        FastCaptureErrorCode Initialize(const std::uint32_t timeout_ms);
        FastCaptureErrorCode RecreateGlRcAndShareContextFrom(HGLRC gl_rc);
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_WGL_CONTEXT_H
