#version 410 core

in vec3 Color;
in vec2 Texcoord;

out vec4 outColor;

uniform vec2 minUV;
uniform vec2 sizeUV;
uniform sampler2D tex;

void main() {
	vec2 nCoord = minUV + sizeUV * Texcoord;
	vec4 col = texture(tex, nCoord) * vec4(Color, 1.0);
	outColor = col;
}

