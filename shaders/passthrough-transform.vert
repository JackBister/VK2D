#version 420 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texcoord;


layout (std140, binding = 0) uniform numbers {
	mat4 pvm;
	vec2 minUV;
	vec2 sizeUV;
};

out vec3 Color;
out vec2 Texcoord;

void main() {
	//floor aligns the quad to the pixel grid
	//gl_Position = projection * floor(view * model * vec4(pos, 1.0));
	gl_Position = pvm * vec4(pos, 1.0);
	Color = color;
	Texcoord = texcoord;
}

