#version 420 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 Texcoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outNormals;

layout (set = 1, binding = 1) uniform sampler2D tex;

void main() {
	vec4 col = texture(tex, Texcoord) * vec4(Color, 1.0);
	outColor = col;
}
