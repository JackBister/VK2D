#version 420 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 Texcoord;

void main() {
	gl_FragDepth = gl_FragCoord.z;
}
