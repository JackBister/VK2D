#pragma once

#include "GL/glew.h"

struct AccessorRendererData
{
	GLuint vao;
};

struct BufferRendererData
{
	GLuint buffer = 0;
};

struct FramebufferRendererData
{
	GLuint framebuffer = 0;

	FramebufferRendererData() {}
	FramebufferRendererData(GLuint fb) : framebuffer(fb) {}
};

struct ImageRendererData
{
	GLuint texture = 0;
};

struct ProgramRendererData
{
	GLuint program = 0;
};

struct ShaderRendererData
{
	GLuint shader = 0;
};
