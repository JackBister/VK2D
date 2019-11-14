#ifdef USE_OGL_RENDERER
#include "OpenGLConverterFuncs.h"

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("OpenGLConverterFuncs");

GLint ToGLAddressMode(AddressMode addressMode)
{
    switch (addressMode) {
    case AddressMode::REPEAT:
        return GL_REPEAT;
    case AddressMode::MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    case AddressMode::CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case AddressMode::CLAMP_TO_BORDER:
        return GL_CLAMP_TO_BORDER;
    case AddressMode::MIRROR_CLAMP_TO_EDGE:
        return GL_MIRROR_CLAMP_TO_EDGE;
    default:
        logger->Errorf("Unrecognized sampler address mode: %d", ToUnderlyingType(addressMode));
        assert(false);
        return GL_REPEAT;
    }
}

GLenum ToGLCullMode(CullMode cullMode)
{
    switch (cullMode) {
    case CullMode::NONE:
        return GL_NONE;
    case CullMode::BACK:
        return GL_BACK;
    case CullMode::FRONT:
        return GL_FRONT;
    case CullMode::FRONT_AND_BACK:
        return GL_FRONT_AND_BACK;
    default:
        assert(false);
        return GL_NONE;
    }
}

GLint ToGLFilter(Filter filter)
{
    switch (filter) {
    case Filter::LINEAR:
        return GL_LINEAR;
    case Filter::NEAREST:
        return GL_NEAREST;
    default:
        logger->Errorf("Unrecognized sampler filter: %d", ToUnderlyingType(filter));
        assert(false);
        return GL_LINEAR;
    }
}

GLint ToGLFormat(Format format)
{
    switch (format) {
    case Format::R8:
        return GL_R;
    case Format::RG8:
        return GL_RG;
    case Format::RGB8:
        return GL_RGB;
    case Format::RGBA8:
        return GL_RGBA;
    default:
        logger->Errorf("Unrecognized image format: %d", ToUnderlyingType(format));
        assert(false);
        return GL_RGBA;
    }
}

GLenum ToGLFrontFace(FrontFace frontFace)
{
    switch (frontFace) {
    case FrontFace::CLOCKWISE:
        return GL_CW;
    case FrontFace::COUNTER_CLOCKWISE:
        return GL_CCW;
    default:
        assert(false);
        return GL_CW;
    }
}

GLint ToGLInternalFormat(Format format)
{
    switch (format) {
    case Format::R8:
        return GL_R8;
    case Format::RG8:
        return GL_RG8;
    case Format::RGB8:
        return GL_RGB8;
    case Format::RGBA8:
        return GL_RGBA8;
    default:
        logger->Errorf("Unrecognized image format: %d", ToUnderlyingType(format));
        assert(false);
        return GL_RGBA8;
    }
}

#endif
