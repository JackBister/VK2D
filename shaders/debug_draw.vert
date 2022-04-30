#version 460
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

layout (std140, set = 0, binding = 0) uniform camera {
	mat4 p;
	mat4 v;
};

layout (location = 0) out vec3 Color;

void main() {
	gl_Position = p * v * vec4(pos, 1.0);
	Color = color;

	gl_PointSize = 10.0;

	//TODO: In the future this should be changed to be the other way around so OpenGL is the one getting penalized
	if (gfxApi == GFX_API_VULKAN) {
		gl_Position.y = -gl_Position.y;
	}
}

