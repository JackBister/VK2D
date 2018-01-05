#version 420 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 Texcoord;

layout (location = 0) out vec4 outColor;

layout (std140, binding = 0) uniform numbers {
	mat4 pvm;
	vec2 minUV;
	vec2 sizeUV;
};
layout (binding = 1) uniform sampler2D tex;


void main() {
	vec2 nCoord = minUV + sizeUV * Texcoord;
	vec4 col = texture(tex, nCoord) * vec4(Color, 1.0);
	outColor = col;
}

