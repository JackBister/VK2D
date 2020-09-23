#version 460 
#extension GL_GOOGLE_include_directive : require

#include "Lights.glsl"
#include "Normals.glsl"
#include "Specialization.glsl"

layout (early_fragment_tests) in;
layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Texcoord;
layout (location = 3) in vec3 WorldPos;
layout (location = 4) in mat4 WorldTransform;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outNormal;

layout (std140, set = 1, binding = 0) uniform camera {
	mat4 pv;
	vec3 cameraPos;
};

layout (set = 3, binding = 0) uniform sampler2D albedo;
layout (set = 3, binding = 1) uniform sampler2D normals;
layout (set = 3, binding = 2) uniform sampler2D roughness;
layout (set = 3, binding = 3) uniform sampler2D metallic;

vec3 GetNormalFromMap(vec3 normalFromTexture)
{
    vec3 tangentNormal = normalFromTexture * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(Texcoord);
    vec2 st2 = dFdy(Texcoord);

    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
	outNormal = encode(Normal);
	
	/*
	// TODO: For some reason the specular is way too "noisy" when using the normal map. Is this just an asset problem?
	vec3 normalFromTexture = texture(normals, Texcoord).xyz;
	vec3 N;
	if (normalFromTexture.x == 0 && normalFromTexture.y == 0 && normalFromTexture.z == 0) {
		N = Normal;
	} else {
		N = GetNormalFromMap(normalFromTexture);
	}
	*/
	vec3 N = Normal;
	vec3 V = normalize(cameraPos - WorldPos);
	vec3 albedo = pow(texture(albedo, Texcoord).xyz, vec3(2.2));
	float metallic = texture(metallic, Texcoord).x;
	float roughness = texture(roughness, Texcoord).x;
	float ao = 0.01;
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = CalculateLight(WorldPos, N, V, albedo, metallic, roughness, ao, F0);

	vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;
	
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0);
}
