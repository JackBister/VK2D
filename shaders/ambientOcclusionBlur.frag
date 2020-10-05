#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 1) in vec2 Texcoord;

layout (set = 0, binding = 0) uniform sampler2D occlusion;

layout (location = 0) out vec4 outColor;

void main() {
	vec2 texelSize = 1.0 / vec2(textureSize(occlusion, 0));

	float result = 0.0;
	for (int x = -2; x < 2; ++x) {
		for (int y = -2; y < 2; ++y) {
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(occlusion, Texcoord + offset).r;
		}
	}

	result /= 16.0;
	outColor = vec4(result, result, result, 1.0);
}
