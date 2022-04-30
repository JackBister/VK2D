#pragma once

#include <atomic>
#include <unordered_map>
#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>

#include "Core/Rendering/BufferSlice.h"
#include "Core/Rendering/Vertex.h"
#include "ParticleEmitter.h"

class BufferAllocator;
class BufferHandle;
class CameraInstance;
class CommandBuffer;
class DebugDrawSystem;
class DescriptorSet;
class DescriptorSetLayoutHandle;
class FrameContext;
class PipelineLayoutHandle;
class Renderer;
class SamplerHandle;
class ShaderProgram;

struct Particle {
    float angularVelocity;
    glm::vec3 velocity;
    float rotation;
    glm::vec3 position;
    float remainingLifeSeconds;
    glm::vec2 size;

    uint32_t isAlive;
    uint32_t imageIdx;
};

struct EmitterUboData {
    glm::mat4 localToWorld;
};

struct ParticleSsboData {
    glm::vec3 position;
    float rotation;

    glm::vec2 size;
    float _padding[2];
};

struct EmitterGpuHandles {
    struct PerFrame {
        DescriptorSet * descriptorSet;

        BufferSlice emitterUbo;
        EmitterUboData * emitterUboMapped;

        BufferSlice particlesSsbo;
        ParticleSsboData * particlesSsboMapped;
    };

    std::vector<PerFrame> perFrame;
};

class ParticleSystem
{
public:
    static ParticleSystem * GetInstance();

    ParticleSystem(Renderer *, BufferAllocator *, DebugDrawSystem *);

    void Init();

    void Tick(float dt);

    void PreRender(FrameContext const &, std::vector<UpdateParticleEmitter> const &);
    void Render(FrameContext const &, CameraInstance const &, CommandBuffer *);

    std::optional<ParticleEmitterId> AddEmitter(ParticleEmitter const &);
    std::optional<ParticleEmitter const> GetEmitter(ParticleEmitterId id);
    void RemoveEmitter(ParticleEmitterId id);

    bool IsDebugDrawEnabled() const { return isDebugDrawEnabled; }
    void SetDebugDrawEnabled(bool enabled) { this->isDebugDrawEnabled = enabled; }

private:
    static ParticleSystem * instance;

    std::atomic_uint64_t currentEmitterId{0};
    std::unordered_map<ParticleEmitterId, ParticleEmitter> emitters;
    std::unordered_map<ParticleEmitterId, std::vector<Particle>> particles;
    std::unordered_map<ParticleEmitterId, EmitterGpuHandles> emitterGpuHandles;

    bool isDebugDrawEnabled = false;

    Renderer * renderer;
    BufferAllocator * bufferAllocator;
    DebugDrawSystem * debugDrawSystem;

    BufferSlice quadVbo;
    BufferSlice quadEbo;
    DescriptorSetLayoutHandle * emitterLayout;
    PipelineLayoutHandle * emitterPipelineLayout;
    SamplerHandle * sampler;
    ShaderProgram * localSpaceParticleRenderingProgram;
    ShaderProgram * worldSpaceParticleRenderingProgram;
};
