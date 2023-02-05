#ifndef FAST_CAPTURE_WINDOWS_IMPL_H
#define FAST_CAPTURE_WINDOWS_IMPL_H

#include "FastCaptureDef.h"
#include <string>

FAST_CAPTURE_NAMESPACE
{
    namespace Windows
    {
        FastCaptureErrorCode InjectProcessImpl(
            const wchar_t& w_process_name,
            const std::wstring& shared_memory_name_prefix) FAST_CAPTURE_NOEXCEPT;
    }
}

#endif // FAST_CAPTURE_WINDOWS_IMPL_H
