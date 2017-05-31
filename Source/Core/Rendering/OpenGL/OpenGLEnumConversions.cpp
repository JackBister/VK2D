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
