#version 460
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#define MAX_VERTEX_BONES 4

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texcoord;
layout (location = 4) in uvec4 boneIds;
layout (location = 5) in vec4 weights;

layout (std140, set = 0, binding = 0) uniform camera {
	mat4 pv;
};
layout (std140, set = 1, binding = 0) uniform model {
	mat4 m[16];
};
layout (std140, set = 2, binding = 0) uniform bones {
    mat4 transformations[256];
};

layout (location = 0) out vec3 Color;
layout (location = 1) out vec2 Texcoord;

void main() {
    mat4 accumulatedTransform = mat4(0.0);
    for (int i = 0; i < MAX_VERTEX_BONES; ++i) {
        if (boneIds[i] == UINT32_MAX) {
            break;
        }
        accumulatedTransform += weights[i] * transformations[boneIds[i]];
    }

	mat4 pvm = pv * m[gl_BaseInstance];
	gl_Position = pvm * (accumulatedTransform * vec4(pos, 1.0));
	Color = color;
	Texcoord = texcoord;

	//TODO: In the future this should be changed to be the other way around so OpenGL is the one getting penalized
	if (gfxApi == GFX_API_VULKAN) {
		gl_Position.y = -gl_Position.y;
	}
}
