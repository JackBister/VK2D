#version 410 core

in vec2 pos;
in vec3 color;
in vec2 texcoord;

out vec3 Color;
out vec2 Texcoord;

void main() {
	gl_Position = vec4(pos, -1.0, 1.0);
	Color = color;
	Texcoord = texcoord;
}
