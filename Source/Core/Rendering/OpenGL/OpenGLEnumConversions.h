#pragma once

#include <gl/glew.h>

#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Image.h"
#include "Core/Rendering/Shader.h"

GLenum AttachmentGL(Framebuffer::Attachment);
GLint FormatGL(Image::Format);
GLint InternalFormatGL(Image::Format);
GLint MinFilterGL(Image::MinFilter);
GLint MagFilterGL(Image::MagFilter);
GLint ShaderTypeGL(Shader::Type);
GLint WrapModeGL(Image::WrapMode);
