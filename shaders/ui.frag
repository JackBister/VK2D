#version 420 
#extension GL_GOOGLE_include_directive : require

#include "Specialization.h"

layout (location = 0) in vec4 Color;
layout (location = 1) in vec2 Texcoord;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 outColor;

void main() {
   outColor = Color * texture(tex, Texcoord);
}
