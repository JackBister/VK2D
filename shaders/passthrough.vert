#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec2 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texcoord;

layout (location = 0) out vec3 Color;
layout (location = 1) out vec2 Texcoord;

void main() {
	gl_Position = vec4(pos, -1.0, 1.0);
	Color = color;
	Texcoord = texcoord;

	//TODO: In the future this should be changed to be the other way around so OpenGL is the one getting penalized
	if (gfxApi == GFX_API_VULKAN) {
		gl_Position.y = -gl_Position.y;
		gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	}
}
