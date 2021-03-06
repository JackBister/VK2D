#ifdef USE_OGL_RENDERER
#include "OpenGLConverterFuncs.h"

#include "Logging/Logger.h"

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
        logger.Error("Unrecognized sampler address mode: {}", ToUnderlyingType(addressMode));
        assert(false);
        return GL_REPEAT;
    }
}

GLenum ToGLCompareOp(CompareOp compareOp)
{
    switch (compareOp) {
    case CompareOp::NEVER:
        return GL_NEVER;
    case CompareOp::LESS:
        return GL_LESS;
    case CompareOp::EQUAL:
        return GL_EQUAL;
    case CompareOp::LESS_OR_EQUAL:
        return GL_LEQUAL;
    case CompareOp::GREATER:
        return GL_GREATER;
    case CompareOp::NOT_EQUAL:
        return GL_NOTEQUAL;
    case CompareOp::GREATER_OR_EQUAL:
        return GL_GEQUAL;
    case CompareOp::ALWAYS:
        return GL_ALWAYS;
    default:
        logger.Warn("Unknown compareOp {}", compareOp);
        return GL_LESS;
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
        logger.Error("Unrecognized sampler filter: {}", ToUnderlyingType(filter));
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
    case Format::R16G16_SFLOAT:
        return GL_RG;
    case Format::R32G32B32A32_SFLOAT:
        return GL_RGBA;
    case Format::D32_SFLOAT:
        return GL_DEPTH_COMPONENT32F;
    default:
        logger.Error("Unrecognized image format: {}", ToUnderlyingType(format));
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
    case Format::R16G16_SFLOAT:
        return GL_RG16;
    case Format::R32G32B32A32_SFLOAT:
        return GL_RGBA32F;
    case Format::D32_SFLOAT:
        return GL_DEPTH_COMPONENT32F;
    case Format::R32_SFLOAT:
        return GL_R32F;
    default:
        logger.Error("Unrecognized image format: {}", ToUnderlyingType(format));
        assert(false);
        return GL_RGBA8;
    }
}

GLenum ToGLPrimitiveTopology(PrimitiveTopology topology)
{
    switch (topology) {
    case PrimitiveTopology::LINE_LIST:
        return GL_LINES;
    case PrimitiveTopology::POINT_LIST:
        return GL_POINTS;
    case PrimitiveTopology::TRIANGLE_LIST:
        return GL_TRIANGLES;
    default:
        logger.Error("Unrecognized primitive topology: {}", ToUnderlyingType(topology));
        assert(false);
        return GL_TRIANGLES;
    }
}

#endif
