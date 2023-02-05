#include "GLCapture.h"
#include <optional>
#include "../Utils/Utils.hpp"
#include "GL/gl.h"

FAST_CAPTURE_NAMESPACE
{
    namespace Details
    {
        struct OptionalVerifier
        {
            inline bool operator()(auto&& opt_value) const noexcept
            {
                return opt_value;
            }
        };
        /**
         * @brief 不应该直接构造此类，请用MakeAutoRecoveryGlTexture2D
         *
         */
        using AutoRecoveryGlTexture2DId =
            Utils::RAIIWrapper<
                std::optional<GLuint>,
                decltype([](auto&& opt_id)
                         {
                           if (opt_id)
                           {
                               ::glBindTexture(GL_TEXTURE_2D, opt_id.value());
                            } }),
                OptionalVerifier>;
    }
    auto MakeAutoRecoveryGlTexture2D()
        ->Details::AutoRecoveryGlTexture2DId
    {
        GLint old_id{-1};
        ::glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_id);
        if (old_id == -1)
            [[unlikely]]
        {
            return {};
        }
        return {old_id};
    }
    class AutoRecoveryGlTexture2DId
    {
        AutoRecoveryGlTexture2DId()
            : impl_{MakeAutoRecoveryGlTexture2D()}
        {
        }

    private:
        Details::AutoRecoveryGlTexture2DId impl_;
    };

    GLCapture::GLCapture() = default;
    GLCapture::~GLCapture() = default;
    // 在Windows下共享上下文
    // https://stackoverflow.com/questions/64271775/sharing-opengl-context-on-windows
    void GLCapture::operator()(GLuint target_texture_id) const noexcept
    {
    }
}
