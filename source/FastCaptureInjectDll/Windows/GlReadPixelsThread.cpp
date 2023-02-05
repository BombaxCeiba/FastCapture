#include "GlReadPixelsThread.h"
#include "WglContext.h"

FAST_CAPTURE_NAMESPACE
{
    FastCaptureErrorCode GlReadPixelsThread::InitializeSignals()
    {
        h_is_thread_init_finish_ = ::CreateEvent(
            nullptr,
            TRUE,
            FALSE,
            nullptr);
        if (h_is_thread_init_finish_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_CREATE_READ_PIXELS_THREAD_INIT_EVENT_FAILED);
        }

        h_is_need_run_command_ = ::CreateEvent(
            nullptr,
            TRUE,
            FALSE,
            nullptr);
        if (h_is_need_run_command_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_CREATE_READ_PIXELS_THREAD_RUN_COMMAND_EVENT_FAILED);
        }

        return FastCaptureMakeSuccessValue();
    }

    FastCaptureErrorCode GlReadPixelsThread::InitializeThread(const std::uint32_t timeout_ms)
    {
        thread_init_timeout_ms_ = timeout_ms;
        h_read_pixels_thread_ = ::CreateThread(
            nullptr,
            0,
            &Do,
            this,
            0,
            nullptr);
        if (h_read_pixels_thread_.IsInvalid())
        {
            return Windows::MakeError(FAST_CAPTURE_E_CREATE_READ_PIXELS_THREAD_FAILED);
        }

        auto wait_result = ::WaitForSingleObject(
            h_read_pixels_thread_.Get(),
            timeout_ms);
        switch (wait_result)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            return Windows::MakeError(FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_TIMEOUT);
        case WAIT_FAILED:
            return Windows::MakeError(FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_FAILED);
        default:
            return Windows::MakeError(FAST_CAPTURE_E_WAIT_RESULT_UNEXPECTED);
        }

        if (!Utils::IsOk(thread_execute_result_))
        {
            return thread_execute_result_;
        }

        return FastCaptureMakeSuccessValue();
    }

    FastCaptureErrorCode GlReadPixelsThread::InvokeThread() const
    {
        if (!::SetEvent(h_is_need_run_command_.Get()))
        {
            return Windows::MakeError(FAST_CAPTURE_E_READ_PIXELS_THREAD_INVOKE_FAILED);
        }
        return FastCaptureMakeSuccessValue();
    }

    DWORD WINAPI GlReadPixelsThread::Do(LPVOID lpThreadParameter)
    {
        auto const p_this = reinterpret_cast<GlReadPixelsThread*>(lpThreadParameter);
        auto& error_code = p_this->thread_execute_result_;

        Windows::FreeLibraryAndExitThreadGuard free_library_and_exit_thread_guard{nullptr};
        WglContext thread_wgl_context{};
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

            error_code = thread_wgl_context.Initialize(p_this->thread_init_timeout_ms_);
            if (!Utils::IsOk(error_code))
            {
                return error_code.error_code;
            }
            error_code = thread_wgl_context.RecreateGlRcAndShareContextFrom(p_this->current_shared_gl_rc_);
            if (!Utils::IsOk(error_code))
            {
                return error_code.error_code;
            }
        }
        do
        {
            switch (::WaitForSingleObject(
                p_this->h_is_need_run_command_.Get(),
                INFINITE))
            {
            case WAIT_OBJECT_0:
                [[likely]] break;
            case WAIT_TIMEOUT:
                error_code = Windows::MakeError(FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_TIMEOUT);
                return error_code.error_code;
            case WAIT_FAILED:
                error_code = Windows::MakeError(FAST_CAPTURE_E_WAIT_INIT_READ_PIXELS_THREAD_FAILED);
                return error_code.error_code;
            default:
                error_code = Windows::MakeError(FAST_CAPTURE_E_WAIT_RESULT_UNEXPECTED);
                return error_code.error_code;
            }
            switch (p_this->command_)
            {
            case Command::ReadPixels:
                [[likely]]
                // TODO: read pixels
                break;
            case Command::ChangeSharedGlRc:
                error_code = thread_wgl_context.RecreateGlRcAndShareContextFrom(p_this->current_shared_gl_rc_);
                if (!Utils::IsOk(error_code))
                {
                    return error_code.error_code;
                }
                break;
            case Command::Exit:
                error_code = FastCaptureMakeSuccessValue();
                return error_code.error_code;
            }
        } while (true);
    }

    FastCaptureErrorCode GlReadPixelsThread::Initialize(const std::uint32_t timeout_ms)
    {
        FastCaptureErrorCode result;
        result = InitializeSignals();
        result = InitializeThread(timeout_ms);
        return result;
    }

    FastCaptureErrorCode GlReadPixelsThread::RequestStopThread()
    {
        SetCommand(Command::Exit);
        return InvokeThread();
    }

    void GlReadPixelsThread::SetCommand(const Command command)
    {
        if (command_ != Command::Exit)
            [[likely]]
        {
            command_ = command;
        }
    }
}