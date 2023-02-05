#ifndef FAST_CAPTURE_INJECT_DLL_INJECT_DLL_DEF_H
#define FAST_CAPTURE_INJECT_DLL_INJECT_DLL_DEF_H

#include <cstddef>
#include <mutex>
#include "FastCaptureDef.h"
#include "GL/glew.h"

FAST_CAPTURE_NAMESPACE
{
    /**
     * @brief 捕获图像的描述信息，读写前必须加锁
     *
     */
    struct CaptureDescriptor
    {
        std::mutex lock{};
        GLint viewport[4]{};
        GLint color_size{};
        struct CaptureImage* p_capture_image{};
        FastCaptureErrorCode wgl_swap_buffers_fake_last_error = FastCaptureMakeSuccessValue();
        FastCaptureErrorCode swap_buffers_fake_last_error = FastCaptureMakeSuccessValue();

        GLint GetWidth() const noexcept
        {
            return viewport[2];
        }
        GLint GetHeight() const noexcept
        {
            return viewport[3];
        }
    };

    /**
     * @brief 捕获图像的数据，读写前必须加锁
     *
     */
    struct CaptureImage
    {
        std::mutex lock{};
        /**
         * @brief 实际上在lock后还有一个成员std::byte data[];
            但是C++不支持柔性数组，因此在这里不写出，而是通过后移指针来实现。
         *
         * @return const std::byte* 移动到lock之后的指针
         */
        const std::byte* GetDataPointer() noexcept
        {
            return reinterpret_cast<const std::byte*>(this) + sizeof(lock);
        }
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_INJECT_DLL_DEF_H
