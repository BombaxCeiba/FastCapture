#ifndef FAST_CAPTURE_UTILS_GL_UTILS_HPP
#define FAST_CAPTURE_UTILS_GL_UTILS_HPP

#include "Utils.hpp"
#include <optional>
#include "GL/glew.h"
#include "GL/gl.h"

FAST_CAPTURE_NAMESPACE
{
    struct OpenGLFboDeleter
    {
        void operator()(const std::optional<GLuint> opt_fbo_id) const noexcept
        {
            if (opt_fbo_id.has_value())
            {
                ::glDeleteFramebuffers(1, &opt_fbo_id.value());
            }
        }
    };
    struct OptGLuintVerifier
    {
        bool operator()(const std::optional<GLuint> opt_value) const noexcept
        {
            return !opt_value.has_value();
        }
    };

    namespace Details
    {
        using UniqueOpenGLFboBase =
            Utils::RAIIWrapper<
                std::optional<GLuint>,
                OpenGLFboDeleter,
                OptGLuintVerifier>;
    }

    class UniqueOpenGLFbo : public Details::UniqueOpenGLFboBase
    {
        using Base = Details::UniqueOpenGLFboBase;

    public:
        using Base::Base;

        GLuint Get() const noexcept
        {
            return Base::GetRef().value();
        }
    };

    inline UniqueOpenGLFbo MakeUniqueOpenGLFbo() noexcept
    {
        GLuint fbo_id[1];
        ::glGenFramebuffers(1, fbo_id);
        return UniqueOpenGLFbo{fbo_id[0]};
    }
}

#endif // FAST_CAPTURE_UTILS_GL_UTILS_HPP
