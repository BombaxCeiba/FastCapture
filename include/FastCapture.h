#ifndef FAST_CAPTURE_H
#define FAST_CAPTURE_H
#include <FastCaptureDef.h>

struct IFastCaptureClient
{
    virtual FastCaptureErrorCode FAST_CAPTURE_CALL
    RequestLatestCaptureSize(size_t* size) FAST_CAPTURE_NOEXCEPT = 0;
    virtual FastCaptureErrorCode FAST_CAPTURE_CALL
    CopyLatestCapture(char* p_memory, size_t memory_size) FAST_CAPTURE_NOEXCEPT = 0;
};

FAST_CAPTURE_EXPORT
IFastCaptureClient* CreateFastCaptureInstance(const wchar_t* const w_process_name) FAST_CAPTURE_NOEXCEPT;
FAST_CAPTURE_EXPORT
FastCaptureErrorCode DestroyFastCaptureInstance(IFastCaptureClient* client) FAST_CAPTURE_NOEXCEPT;

#endif
