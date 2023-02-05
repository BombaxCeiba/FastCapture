#ifndef FAST_CAPTURE_INJECT_DLL_GL_CAPTURE_H
#define FAST_CAPTURE_INJECT_DLL_GL_CAPTURE_H

#include "FastCaptureDef.h"
#include "GL/glew.h"

FAST_CAPTURE_NAMESPACE
{
    /**
     * @brief 此类用于在被Hook的名为类似于SwapBuffer的函数中构造、执行括号重载和析构，
        例如：GLCapture{}();
        在这个过程中，它备份必要的OpenGL状态信息，复制当前fbo对象的内容到一个自己创建的纹理中，
        再恢复改动过的OpenGL状态信息
     *
     */
    class GLCapture
    {
    public:
        GLCapture();
        ~GLCapture();
        void operator()(GLuint target_texture_id) const noexcept;

    private:
        class AutoRecoveryGlTexture2DId;
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_GL_CAPTURE_H
