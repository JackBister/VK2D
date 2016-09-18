#version 410 core

in vec3 pos;
in vec3 color;
in vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 Texcoord;
out vec3 Color;

void main() {
	//floor aligns the quad to the pixel grid
	gl_Position = projection * floor(view * model * vec4(pos, 1.0));
	//gl_Position = projection * view * model * vec4(pos, 1.0);
	Color = color;
	Texcoord = texcoord;
}

