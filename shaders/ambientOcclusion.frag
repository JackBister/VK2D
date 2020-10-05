#version 420
#extension GL_GOOGLE_include_directive : require

#include "Normals.glsl"
#include "Specialization.glsl"

const int MAX_SAMPLES = 64;
const float RADIUS = 20.0;
const float BIAS = 0.025;

layout (location = 1) in vec2 Texcoord;

layout (set = 0, binding = 0) uniform sampler2D normals;
layout (set = 0, binding = 1) uniform sampler2D depth;
layout (set = 0, binding = 2) uniform sampler2D ssaoNoise;
layout (std140, set = 0, binding = 3) uniform ssaoParameters {
	vec3 samples[MAX_SAMPLES];
	vec2 screenResolution;
	mat4 projection;
	mat4 view;
};

layout (location = 0) out vec4 outColor;

vec3 DepthToViewSpace(float depthCS) {
	vec3 positionCS = vec3(Texcoord.x * 2.0 - 1.0, (1.0 - Texcoord.y) * 2.0 - 1.0, depthCS);
	vec4 positionVS = inverse(projection) * vec4(positionCS, 1.0);
	positionVS /= positionVS.w;
	return positionVS.xyz;
}

void main() {
	vec2 noiseScale = vec2(screenResolution.x / 4.0, screenResolution.y / 4.0);

	vec3 normalWS = decode(texture(normals, Texcoord).xy);
	float depthCS = texture(depth, Texcoord).x;
	vec3 positionVS = DepthToViewSpace(depthCS);

    vec3 randomVec = normalize(texture(ssaoNoise, Texcoord * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normalWS * dot(randomVec, normalWS));
    vec3 bitangent = cross(normalWS, tangent);
    mat3 TBN = mat3(tangent, bitangent, normalWS);

	float occlusion = 0.0;
	vec3 sample0 = samples[0];
	vec3 sample0tbn = TBN * samples[0];
	for (int i = 0; i < MAX_SAMPLES; ++i) {
		vec3 sampleVS = TBN * samples[i];
		vec3 sampleVSbeforeAdd = sampleVS * RADIUS;
		sampleVS = positionVS + sampleVS * RADIUS;

		vec4 offsetCS = vec4(sampleVS, 1.0);
		offsetCS = projection * offsetCS;
	
		offsetCS.xyz /= offsetCS.w;
		offsetCS.xyz = offsetCS.xyz * 0.5 + 0.5;
	
		if (gfxApi == GFX_API_VULKAN) {
			offsetCS.y = 1.0 - offsetCS.y;
		}	

		float sampleDepthCS = texture(depth, offsetCS.xy).x;
		float sampleZVS = DepthToViewSpace(sampleDepthCS).z;

		float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(positionVS.z - sampleZVS));
		occlusion += (sampleZVS >= sampleVS.z + BIAS ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / MAX_SAMPLES);

	outColor = vec4(occlusion, occlusion, occlusion, 1.0);
}
