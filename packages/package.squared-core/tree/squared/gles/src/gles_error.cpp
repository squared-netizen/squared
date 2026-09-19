#include <squared/gles/gles_error.hpp>

#include <squared/gles/gles_error_code.hpp>

#include <string>

#include <GLES3/gl3.h>

namespace sq::gles {

const char* name_of(GlesErrorCode code) noexcept
{
    switch (code) {
    case GlesErrorCode::None: return "None";
    case GlesErrorCode::NoContext: return "NoContext";
    case GlesErrorCode::OutOfMemory: return "OutOfMemory";
    case GlesErrorCode::CompileFailed: return "CompileFailed";
    case GlesErrorCode::LinkFailed: return "LinkFailed";
    case GlesErrorCode::InvalidArgument: return "InvalidArgument";
    case GlesErrorCode::Unsupported: return "Unsupported";
    case GlesErrorCode::GlError: return "GlError";
    }
    return "?";
}

const char* gl_enum_name(unsigned int value) noexcept
{
    switch (value) {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    case GL_FLOAT: return "GL_FLOAT";
    case GL_FLOAT_VEC2: return "GL_FLOAT_VEC2";
    case GL_FLOAT_VEC3: return "GL_FLOAT_VEC3";
    case GL_FLOAT_VEC4: return "GL_FLOAT_VEC4";
    case GL_FLOAT_MAT2: return "GL_FLOAT_MAT2";
    case GL_FLOAT_MAT3: return "GL_FLOAT_MAT3";
    case GL_FLOAT_MAT4: return "GL_FLOAT_MAT4";
    case GL_INT: return "GL_INT";
    case GL_INT_VEC2: return "GL_INT_VEC2";
    case GL_INT_VEC3: return "GL_INT_VEC3";
    case GL_INT_VEC4: return "GL_INT_VEC4";
    case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT";
    case GL_BOOL: return "GL_BOOL";
    case GL_SAMPLER_2D: return "GL_SAMPLER_2D";
    case GL_SAMPLER_CUBE: return "GL_SAMPLER_CUBE";
    default: return "GL_?";
    }
}

const char* glsl_type_name(unsigned int type) noexcept
{
    switch (type) {
    case GL_FLOAT: return "float";
    case GL_FLOAT_VEC2: return "vec2";
    case GL_FLOAT_VEC3: return "vec3";
    case GL_FLOAT_VEC4: return "vec4";
    case GL_FLOAT_MAT2: return "mat2";
    case GL_FLOAT_MAT3: return "mat3";
    case GL_FLOAT_MAT4: return "mat4";
    case GL_INT: return "int";
    case GL_INT_VEC2: return "ivec2";
    case GL_INT_VEC3: return "ivec3";
    case GL_INT_VEC4: return "ivec4";
    case GL_UNSIGNED_INT: return "uint";
    case GL_BOOL: return "bool";
    case GL_SAMPLER_2D: return "sampler2D";
    case GL_SAMPLER_CUBE: return "samplerCube";
    default: return "?";
    }
}

GlesError check_gl_errors() noexcept
{
    GLenum first = GL_NO_ERROR;
    GLenum current = glGetError();

    // Drain the whole queue. GL hands errors back one at a time, and leaving
    // any queued makes the next unrelated check report this failure instead.
    while (current != GL_NO_ERROR) {
        if (first == GL_NO_ERROR) first = current;
        current = glGetError();
    }

    if (first == GL_NO_ERROR) return {};

    return GlesError{
        .code = first == GL_OUT_OF_MEMORY ? GlesErrorCode::OutOfMemory
                                          : GlesErrorCode::GlError,
        .message = gl_enum_name(first)
    };
}

}  // namespace sq::gles
