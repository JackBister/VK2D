#version 460 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec3 Color;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(Color, 1.0);
}
