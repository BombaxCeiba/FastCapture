#ifndef FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_H
#define FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_H

#include "FastCaptureDef.h"
#include "../../Utils/Windows/UtilsWindows.hpp"

FAST_CAPTURE_NAMESPACE
{
    class WglContext;

    class GlReadPixelsThread
    {
    public:
        enum class Command
        {
            ReadPixels,
            ChangeSharedGlRc,
            Exit
        };

    private:
        Windows::UniqueHandleInvalidNULL h_read_pixels_thread_{nullptr};
        /**
        * @brief 这是一个AUTO_RESET的，默认值为0的信号量。
           当此信号量变为1时，外部调用的::WaitForSingleObject返回，
           表示线程初始化完成
        *
        */
        Windows::UniqueHandleInvalidNULL h_is_thread_init_finish_{nullptr};
        /**
        * @brief 这是一个AUTO_RESET的，默认值为0的信号量。
           当此信号量变为1时，线程调用的::WaitForSingleObject返回，
           通知线程开始执行::glReadPixel函数
        *
        */
        Windows::UniqueHandleInvalidNULL h_is_need_run_command_{nullptr};
        FastCaptureErrorCode thread_execute_result_{FastCaptureMakeSuccessValue()};
        HGLRC current_shared_gl_rc_{nullptr};
        std::uint32_t thread_init_timeout_ms_{0};
        Command command_{Command::ReadPixels};

        FastCaptureErrorCode InitializeSignals();
        FastCaptureErrorCode InitializeThread(const std::uint32_t timeout_ms);
        FastCaptureErrorCode InvokeThread() const;

        static DWORD WINAPI Do(LPVOID lpThreadParameter);

    public:
        FastCaptureErrorCode Initialize(const std::uint32_t timeout_ms);
        FastCaptureErrorCode RequestStopThread();
        void SetCommand(const Command command);
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_READ_PIXELS_THREAD_H
