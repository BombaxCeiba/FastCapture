#include "FastCapture.h"
#include "Impl.h"
#include "../../Utils/Utils.hpp"

FAST_CAPTURE_NAMESPACE
{
    FastCaptureErrorCode InjectProcess(const wchar_t* const w_process_name, const wchar_t* const w_pipe_name) FAST_CAPTURE_NOEXCEPT
    {
        if (w_process_name == nullptr || w_process_name == nullptr)
        {
            return Utils::MakeError(FAST_CAPTURE_E_INVALID_ARGUMENT);
        }
        std::wstring pipe_name{w_pipe_name};
        return Windows::InjectProcessImpl(*w_process_name, pipe_name);
    }

    class FastCaptureClient : public IFastCaptureClient
    {
    public:
        FastCaptureClient();
        ~FastCaptureClient();
    };
};
