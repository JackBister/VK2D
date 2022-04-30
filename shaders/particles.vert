#version 460
#extension GL_GOOGLE_include_directive : require

#include "Particles.glsl"
#include "Specialization.glsl"

layout (constant_id = 1) const uint isWorldspaceParticleEmitter = 0;

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

layout (std140, set = 0, binding = 0) uniform emitter {
	mat4 m;
};
layout (std140, set = 0, binding = 1) readonly buffer emitterParticles {
	Particle particles[];
};
layout (std140, set = 1, binding = 0) uniform camera {
	mat4 p;
	mat4 v;
};

layout (location = 0) out vec2 Texcoord;
layout (location = 1) out float Rotation;

void main() {
	Particle particle = particles[gl_InstanceIndex];

	vec3 camRightWS = vec3(v[0][0], v[1][0], v[2][0]);
	vec3 camUpWS = vec3(v[0][1], v[1][1], v[2][1]);
	vec3 particleCenterWS;
	if (isWorldspaceParticleEmitter != 0) {
		particleCenterWS = particle.position;
	} else {
		particleCenterWS = (m * vec4(particle.position, 1.0)).xyz;
	}
	vec3 finalPosWS = particleCenterWS + camRightWS * particle.size.x * pos.x + camUpWS * particle.size.y * pos.y;
	gl_Position = p * v * vec4(finalPosWS, 1.0);

	Rotation = particle.rotation;
	Texcoord = uv;
	if (gfxApi == GFX_API_VULKAN) {
		gl_Position.y = -gl_Position.y;
	}
}

