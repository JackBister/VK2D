#version 420 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 Texcoord;

layout (location = 0) out vec4 outColor;

layout (set = 2, binding = 0) uniform sampler2D albedo;

void main() {
	vec4 col = texture(albedo, Texcoord) * vec4(Color, 1.0);
	outColor = col;
}
