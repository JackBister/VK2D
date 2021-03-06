#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 Texcoord;

layout (set = 0, binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 outColor;

void main() {
	vec4 col = texture(tex, Texcoord) * vec4(Color, 1.0);
	outColor = col;
}

