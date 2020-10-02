#version 420
#extension GL_GOOGLE_include_directive : require

#include "Specialization.glsl"

layout (location = 1) in vec2 Texcoord;

layout (set = 0, binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 outColor;

void main() {
	vec3 col = texture(tex, Texcoord).xyz;

    // HDR tonemapping
    col = col / (col + vec3(1.0));
    // gamma correct
    col = pow(col, vec3(1.0/2.2));

	outColor = vec4(col, 1.0);
}

