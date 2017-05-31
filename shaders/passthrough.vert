#version 420

layout (location = 0) in vec2 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texcoord;

out vec3 Color;
out vec2 Texcoord;

void main() {
	gl_Position = vec4(pos, -1.0, 1.0);
	Color = color;
	Texcoord = texcoord;
}
