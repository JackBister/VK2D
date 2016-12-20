#pragma once

#include "GL/glew.h"

struct OpenGLFramebufferRendererData
{
	GLuint framebuffer = 0;
};

struct OpenGLImageRendererData
{
	GLuint texture = 0;
};

struct OpenGLShaderRendererData
{
	GLuint shader = 0;
};
