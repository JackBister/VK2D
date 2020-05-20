#version 460 
#extension GL_GOOGLE_include_directive : require

#include "Normals.h"
#include "Specialization.h"

layout (early_fragment_tests) in;
layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Texcoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outNormal;

layout (set = 2, binding = 0) uniform sampler2D albedo;

void main() {
	vec4 col = texture(albedo, Texcoord) * vec4(Color, 1.0);
	outColor = col;
	outNormal = encode(Normal);
}
