#version 460 
#extension GL_GOOGLE_include_directive : require

#include "Particles.glsl"
#include "Specialization.glsl"

layout (location = 0) in vec2 Texcoord;
layout (location = 1) in float rotation;

layout (set = 0, binding = 2) uniform sampler2D tex;

layout (location = 0) out vec4 outColor;

void main() {
	vec4 col = texture(tex, Texcoord);
	if (col.a == 0) {
		discard;
	}
	outColor = col;
}
