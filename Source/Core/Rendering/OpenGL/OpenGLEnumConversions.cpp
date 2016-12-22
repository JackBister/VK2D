#include "Core/Rendering/OpenGL/OpenGLEnumConversions.h"

template <typename T>
typename std::underlying_type<T>::type ToUnderlyingType(T in)
{
	return static_cast<typename std::underlying_type<T>::type>(in);
}

GLenum AttachmentGL(Framebuffer::Attachment attachment)
{
	switch (attachment) {
	case Framebuffer::Attachment::COLOR0:
		return GL_COLOR_ATTACHMENT0;
		break;
	case Framebuffer::Attachment::DEPTH:
		return GL_DEPTH_ATTACHMENT;
		break;
	case Framebuffer::Attachment::STENCIL:
		return GL_STENCIL_ATTACHMENT;
		break;
	case Framebuffer::Attachment::DEPTH_STENCIL:
		return GL_DEPTH_STENCIL_ATTACHMENT;
		break;
	default:
		printf("[WARNING] Unknown framebuffer attachment %d.", ToUnderlyingType(attachment));
		return GL_COLOR_ATTACHMENT0;
	}
}

GLint FormatGL(Image::Format format)
{
	switch (format) {
	case Image::Format::R8:
		return GL_R;
	case Image::Format::RG8:
		return GL_RG;
	case Image::Format::RGB8:
		return GL_RGB;
	case Image::Format::RGBA8:
		return GL_RGBA;
	default:
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		return GL_RGBA;
	}
}

GLint InternalFormatGL(Image::Format format)
{
	switch (format) {
	case Image::Format::R8:
		return GL_R8;
	case Image::Format::RG8:
		return GL_RG8;
	case Image::Format::RGB8:
		return GL_RGB8;
	case Image::Format::RGBA8:
		return GL_RGBA8;
	default:
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		return GL_RGBA8;
	}
}

GLint MinFilterGL(Image::MinFilter filter)
{
	switch (filter) {
	case Image::MinFilter::LINEAR:
		return GL_LINEAR;
	case Image::MinFilter::LINEAR_MIPMAP_LINEAR:
		return GL_LINEAR_MIPMAP_LINEAR;
	case Image::MinFilter::LINEAR_MIPMAP_NEAREST:
		return GL_LINEAR_MIPMAP_NEAREST;
	case Image::MinFilter::NEAREST:
		return GL_NEAREST;
	case Image::MinFilter::NEAREST_MIPMAP_LINEAR:
		return GL_NEAREST_MIPMAP_LINEAR;
	case Image::MinFilter::NEAREST_MIPMAP_NEAREST:
		return GL_NEAREST_MIPMAP_NEAREST;
	default:
		printf("[ERROR] Unrecognized image min filter: %d\n", ToUnderlyingType(filter));
		return GL_NEAREST_MIPMAP_LINEAR;
	}
}

GLint MagFilterGL(Image::MagFilter filter)
{
	switch (filter) {
	case Image::MagFilter::LINEAR:
		return GL_LINEAR;
	case Image::MagFilter::NEAREST:
		return GL_NEAREST;
	default:
		printf("[ERROR] Unrecognized image mag filter: %d\n", ToUnderlyingType(filter));
		return GL_LINEAR;
	}
	return GLint();
}

GLint ShaderTypeGL(Shader::Type type)
{
	switch (type) {
	case Shader::Type::VERTEX_SHADER:
		return GL_VERTEX_SHADER;
		break;
	case Shader::Type::FRAGMENT_SHADER:
		return GL_FRAGMENT_SHADER;
		break;
	case Shader::Type::GEOMETRY_SHADER:
		return GL_GEOMETRY_SHADER;
		break;
	case Shader::Type::COMPUTE_SHADER:
		return GL_COMPUTE_SHADER;
		break;
	default:
		printf("[ERROR] Unknown shader type: %d\n", ToUnderlyingType(type));
		return GL_VERTEX_SHADER;
	}
}

GLint WrapModeGL(Image::WrapMode mode)
{
	switch (mode) {
	case Image::WrapMode::CLAMP_TO_BORDER:
		return GL_CLAMP_TO_BORDER;
	case Image::WrapMode::CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
	case Image::WrapMode::MIRRORED_REPEAT:
		return GL_MIRRORED_REPEAT;
	case Image::WrapMode::MIRROR_CLAMP_TO_EDGE:
		return GL_MIRROR_CLAMP_TO_EDGE;
	case Image::WrapMode::REPEAT:
		return GL_REPEAT;
	default:
		printf("[ERROR] Unrecognized image wrap mode: %d\n", ToUnderlyingType(mode));
		return GL_REPEAT;
	}
}
