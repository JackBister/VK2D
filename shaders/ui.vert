#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout (location = 0) out vec4 Color;
layout (location = 1) out vec2 Texcoord;

void main()
{
    Color = color;
    Texcoord = uv;
    gl_Position = vec4(pos * vec2(2.0/800.0, 2.0/600.0) - vec2(1.0, 1.0), 0, 1);

	if (gfxApi == GFX_API_OPENGL) {
		gl_Position.y = -gl_Position.y;
		gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	}
}
