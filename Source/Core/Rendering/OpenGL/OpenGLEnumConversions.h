#pragma once

#include <gl/glew.h>

#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Image.h"
#include "Core/Rendering/Shader.h"

GLenum AttachmentGL(Framebuffer::Attachment);
GLint ShaderTypeGL(Shader::Type);