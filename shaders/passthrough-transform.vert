#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texcoord;

layout (std140, set = 0, binding = 0) uniform camera {
	mat4 p;
	mat4 v;
};
layout (std140, set = 1, binding = 0) uniform model {
	mat4 m;
};

layout (location = 0) out vec3 Color;
layout (location = 1) out vec2 Texcoord;

void main() {
	mat4 pvm = p * v * m;
	gl_Position = pvm * vec4(pos, 1.0);
	Color = color;
	Texcoord = texcoord;

	//TODO: In the future this should be changed to be the other way around so OpenGL is the one getting penalized
	if (gfxApi == GFX_API_VULKAN) {
		gl_Position.y = -gl_Position.y;
		gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	}
}

