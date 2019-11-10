#ifdef USE_OGL_RENDERER
#include <cassert>
#include <type_traits>

#include <GL/glew.h>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"

template <typename T>
static typename std::underlying_type<T>::type ToUnderlyingType(T in)
{
	return static_cast<typename std::underlying_type<T>::type>(in);
}

static GLint ToGLAddressMode(AddressMode addressMode)
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
		printf("[ERROR] Unrecognized sampler address mode: %d\n", ToUnderlyingType(addressMode));
		assert(false);
		return GL_REPEAT;
	}
}

static GLenum ToGLCullMode(CullMode cullMode)
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

static GLint ToGLFilter(Filter filter)
{
	switch (filter) {
	case Filter::LINEAR:
		return GL_LINEAR;
	case Filter::NEAREST:
		return GL_NEAREST;
	default:
		printf("[ERROR] Unrecognized sampler filter: %d\n", ToUnderlyingType(filter));
		assert(false);
		return GL_LINEAR;
	}
}

static GLint ToGLFormat(Format format)
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
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		assert(false);
		return GL_RGBA;
	}
}

static GLenum ToGLFrontFace(FrontFace frontFace)
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

static GLint ToGLInternalFormat(Format format)
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
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		assert(false);
		return GL_RGBA8;
	}
}

#endif
