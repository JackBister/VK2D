#include "ParticleSystem.h"

#include <Console/Console.h>
#include <Logging/Logger.h>
#include <RenderingBackend/Renderer.h>
#include <ThirdParty/Optick/src/optick.h>
#include <Util/Semaphore.h>

#include "Core/FrameContext.h"
#include "Core/Rendering/BufferAllocator.h"
#include "Core/Rendering/CameraInstance.h"
#include "Core/Rendering/DebugDrawSystem.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"
#include "Util/Lerp.h"
#include "Util/RandomFloat.h"

static auto const logger = Logger::Create("ParticleSystem");

ParticleSystem * ParticleSystem::instance;

ParticleSystem * ParticleSystem::GetInstance()
{
    return ParticleSystem::instance;
}

ParticleSystem::ParticleSystem(Renderer * renderer, BufferAllocator * bufferAllocator,
                               DebugDrawSystem * debugDrawSystem)
    : renderer(renderer), bufferAllocator(bufferAllocator), debugDrawSystem(debugDrawSystem)
{
    ParticleSystem::instance = this;

    CommandDefinition debugParticlesCommand("debug_particles",
                                            "debug_particles <0 or 1> - enables or disables debug drawing particles",
                                            1,
                                            [this](auto args) {
                                                auto arg = args[0];
                                                if (arg == "0") {
                                                    this->SetDebugDrawEnabled(false);
                                                } else if (arg == "1") {
                                                    this->SetDebugDrawEnabled(true);
                                                }
                                            });
    Console::RegisterCommand(debugParticlesCommand);
}

void ParticleSystem::Init()
{
    emitterLayout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
        "_Primitives/DescriptorSetLayouts/particleEmitter.layout");
    emitterPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/particles.pipelinelayout");
    sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");
    localSpaceParticleRenderingProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/particleRendering.program");
    worldSpaceParticleRenderingProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/particleRendering_worldSpace.program");

    quadVbo = bufferAllocator->AllocateBuffer(4 * sizeof(ParticleVertex),
                                              BufferUsageFlags::TRANSFER_DST_BIT | BufferUsageFlags::VERTEX_BUFFER_BIT,
                                              MemoryPropertyFlagBits::DEVICE_LOCAL_BIT);
    quadEbo = bufferAllocator->AllocateBuffer(6 * sizeof(uint32_t),
                                              BufferUsageFlags::TRANSFER_DST_BIT | BufferUsageFlags::INDEX_BUFFER_BIT,
                                              MemoryPropertyFlagBits::DEVICE_LOCAL_BIT);

    Semaphore sem;
    renderer->CreateResources([this, &sem](ResourceCreationContext & ctx) {
        std::array<float, 20> quadVboContent{
            // clang-format off
            // vec2 pos, vec2 uv
            -0.5f, 0.5f, 0.0f, 1.0f, // Top left
            0.5f,  0.5f, 1.0f, 1.0f, // Top right
            0.5f,  -0.5f, 1.0f, 0.0f, // Bottom right
            -0.5f, -0.5f, 0.0f, 0.0f, // Bottom left
            // clang-format on
        };
        ctx.BufferSubData(
            quadVbo.GetBuffer(), (uint8_t *)quadVboContent.data(), quadVbo.GetOffset(), 4 * sizeof(ParticleVertex));
        std::array<uint32_t, 6> quadEboContent{0, 1, 2, 2, 3, 0};
        ctx.BufferSubData(
            quadEbo.GetBuffer(), (uint8_t *)quadEboContent.data(), quadEbo.GetOffset(), 6 * sizeof(uint32_t));
        sem.Signal();
    });
    sem.Wait();
}

void ParticleSystem::Tick(float dt)
{
    OPTICK_EVENT();
    for (auto & kv : emitters) {
        if (!kv.second.isActive) {
            continue;
        }
        auto parIt = particles.find(kv.first);
        if (particles.find(kv.first) == particles.end()) {
            logger.Warn("Active particle emitter with id={} does not have any particles in the particles map. Bug?",
                        kv.first);
            continue;
        }
        auto & emitter = kv.second;
        auto & pars = parIt->second;
        float lastTime = emitter.simulationTime;
        emitter.simulationTime += dt;

        glm::vec3 acceleration(0.f);
        for (auto const & a : emitter.accelerators) {
            acceleration += a;
        }

        if (emitter.isEmissionEnabled) {
            float rate = 1.f / emitter.emissionRate;

            emitter.emitCounter += dt;

            size_t lastInactiveIdx = 0;
            while (emitter.numActiveParticles < emitter.numParticles && emitter.emitCounter > rate) {
                size_t lastInactiveBefore = lastInactiveIdx;
                for (size_t i = lastInactiveIdx; i < pars.size(); ++i) {
                    if (!pars[i].isAlive) {
                        lastInactiveIdx = i;
                        goto inactiveFound;
                    }
                }
                if (lastInactiveIdx == lastInactiveBefore) {
                    break;
                }
            inactiveFound:
                emitter.numActiveParticles++;
                pars[lastInactiveIdx].isAlive = 1;
                glm::vec3 velocity =
                    glm::vec3(Lerp(emitter.initialVelocityMin.x, emitter.initialVelocityMax.x, RandomFloat()),
                              Lerp(emitter.initialVelocityMin.y, emitter.initialVelocityMax.y, RandomFloat()),
                              Lerp(emitter.initialVelocityMin.z, emitter.initialVelocityMax.z, RandomFloat()));
                pars[lastInactiveIdx].velocity = velocity;
                pars[lastInactiveIdx].rotation = 0.f;
                if (emitter.isWorldSpace) {
                    pars[lastInactiveIdx].position =
                        glm::vec3(emitter.localToWorld[3][0], emitter.localToWorld[3][1], emitter.localToWorld[3][2]);
                } else {
                    pars[lastInactiveIdx].position = glm::vec3(0.f);
                }
                pars[lastInactiveIdx].remainingLifeSeconds = emitter.lifetime;
                float sizeX = Lerp(emitter.initialSizeMin.x, emitter.initialSizeMax.x, RandomFloat());
                if (emitter.alwaysSquareSize) {
                    pars[lastInactiveIdx].size = glm::vec2(sizeX, sizeX);
                } else {
                    pars[lastInactiveIdx].size =
                        glm::vec2(sizeX, Lerp(emitter.initialSizeMin.y, emitter.initialSizeMax.y, RandomFloat()));
                }
                pars[lastInactiveIdx].angularVelocity = 0.f;

                emitter.emitCounter -= rate;
            }
        }

        for (auto & p : pars) {
            if (!p.isAlive) {
                continue;
            }
            p.remainingLifeSeconds -= dt;
            if (p.remainingLifeSeconds < 0.f) {
                p.isAlive = false;
                emitter.numActiveParticles--;
                continue;
            }
            p.velocity += acceleration * dt;
            p.position += p.velocity * dt;
            p.rotation += p.angularVelocity * dt;
        }
    }
}

void ParticleSystem::PreRender(FrameContext const & context, std::vector<UpdateParticleEmitter> const & updates)
{
    OPTICK_EVENT();
    // TODO: This is all very unsafe since PreRender may run concurrently with Tick
    for (auto const & update : updates) {
        auto emitterIt = emitters.find(update.id);
        if (emitterIt == emitters.end()) {
            logger.Warn("Attempted to update particle emitter with id={} which does not exist. Maybe it has already "
                        "been destroyed?",
                        update.id);
            continue;
        }
        auto gpuHandlesIt = emitterGpuHandles.find(update.id);
        if (gpuHandlesIt == emitterGpuHandles.end()) {
            logger.Warn("Did not find GPU handles for particle emitter with id={} during PreRender. Bug?", update.id);
            continue;
        }
        auto particlesIt = particles.find(update.id);
        if (particlesIt == particles.end()) {
            logger.Warn("Did not find particles for particle emitter with id={} during PreRender. Bug?", update.id);
            continue;
        }
        emitterIt->second.localToWorld = update.localToWorld;
        auto & thisFrame = gpuHandlesIt->second.perFrame[context.currentGpuFrameIndex];
        thisFrame.emitterUboMapped->localToWorld = update.localToWorld;

        size_t ssboIdx = 0;
        // TODO: Particle positions are currently only updated as long as an UpdateParticleEmitter is submitted each
        // frame...? Is this acceptable?
        for (auto & p : particlesIt->second) {
            if (!p.isAlive) {
                continue;
            }
            thisFrame.particlesSsboMapped[ssboIdx].position = p.position;
            thisFrame.particlesSsboMapped[ssboIdx].rotation = p.rotation;
            thisFrame.particlesSsboMapped[ssboIdx].size = p.size;
            ++ssboIdx;
        }
    }
}

void ParticleSystem::Render(FrameContext const & context, CameraInstance const & camera, CommandBuffer * commandBuffer)
{
    OPTICK_EVENT();
    if (isDebugDrawEnabled) {
        for (auto const & kv : emitters) {
            if (!kv.second.isActive) {
                continue;
            }
            auto parIt = particles.find(kv.first);
            if (particles.find(kv.first) == particles.end()) {
                logger.Warn("Active particle emitter with id={} does not have any particles in the particles map. Bug?",
                            kv.first);
                continue;
            }
            auto & emitter = kv.second;
            auto & pars = parIt->second;

            for (auto & p : pars) {
                glm::vec4 worldSpacePosition = emitter.localToWorld * glm::vec4(p.position, 1.f);
                debugDrawSystem->DrawPoint(worldSpacePosition, glm::vec3(1.f, 1.f, 1.f), 0.f);
            }
        }
    }

    commandBuffer->CmdBindVertexBuffer(quadVbo.GetBuffer(), 0, quadVbo.GetOffset(), sizeof(ParticleVertex));
    commandBuffer->CmdBindIndexBuffer(quadEbo.GetBuffer(), quadEbo.GetOffset(), CommandBuffer::IndexType::UINT32);
    commandBuffer->CmdBindDescriptorSets(emitterPipelineLayout, 1, {camera.GetDecsriptorSet()});

    commandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                   localSpaceParticleRenderingProgram->GetPipeline());
    ShaderProgram * currentProgram = localSpaceParticleRenderingProgram;
    for (auto const & emitter : emitters) {
        if (!emitter.second.isActive) {
            continue;
        }

        auto gpuHandlesIt = emitterGpuHandles.find(emitter.first);
        if (gpuHandlesIt == emitterGpuHandles.end()) {
            logger.Warn("Did not find GPU handles for particle emitter with id={} during Render. Bug?", emitter.first);
            continue;
        }

        auto & thisFrame = gpuHandlesIt->second.perFrame[context.currentGpuFrameIndex];

        if (emitter.second.isWorldSpace && currentProgram != worldSpaceParticleRenderingProgram) {
            commandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                           worldSpaceParticleRenderingProgram->GetPipeline());
            currentProgram = worldSpaceParticleRenderingProgram;
        } else if (!emitter.second.isWorldSpace && currentProgram != localSpaceParticleRenderingProgram) {
            commandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                           localSpaceParticleRenderingProgram->GetPipeline());
            currentProgram = localSpaceParticleRenderingProgram;
        }

        commandBuffer->CmdBindDescriptorSets(emitterPipelineLayout, 0, {thisFrame.descriptorSet});

        commandBuffer->CmdDrawIndexed(6, emitter.second.numActiveParticles, 0, quadVbo.GetOffset(), 0);
    }
}

std::optional<ParticleEmitterId> ParticleSystem::AddEmitter(ParticleEmitter const & emitter)
{
    OPTICK_EVENT();
    if (emitter.numParticles == 0) {
        logger.Error("Attempt to create particle emitter with numParticles=0. Will not create this emitter.");
        return std::nullopt;
    }
    if (emitter.image == nullptr) {
        logger.Error("Attempt to create particle emitter with null image. Will not create this emitter.");
        return std::nullopt;
    }
    ParticleEmitterId id = currentEmitterId.fetch_add(1);
    emitters.insert({id, emitter});
    std::vector<Particle> eParticles(emitter.numParticles);
    for (auto & p : eParticles) {
        p.velocity = glm::vec3(0.f);
        p.rotation = 0.f;
        p.position = glm::vec3(0.f);
        p.remainingLifeSeconds = 0.f;
        p.size = glm::vec2(1.f);

        p.isAlive = 0;
        p.imageIdx = 0;
        p.angularVelocity = 0.f;
    }
    particles.insert({id, eParticles});

    EmitterGpuHandles gpuHandles;
    Semaphore sem;
    renderer->CreateResources([this, &emitter, &gpuHandles, &sem](ResourceCreationContext & ctx) {
        auto swapCount = renderer->GetSwapCount();
        for (size_t i = 0; i < swapCount; ++i) {
            BufferSlice emitterUbo = bufferAllocator->AllocateBuffer(sizeof(EmitterUboData),
                                                                     BufferUsageFlags::UNIFORM_BUFFER_BIT,
                                                                     MemoryPropertyFlagBits::HOST_COHERENT_BIT |
                                                                         MemoryPropertyFlagBits::HOST_VISIBLE_BIT);
            EmitterUboData * emitterUboMapped = (EmitterUboData *)bufferAllocator->MapBuffer(emitterUbo);

            BufferSlice particlesSsbo = bufferAllocator->AllocateBuffer(emitter.numParticles * sizeof(ParticleSsboData),
                                                                        BufferUsageFlags::STORAGE_BUFFER_BIT,
                                                                        MemoryPropertyFlagBits::HOST_COHERENT_BIT |
                                                                            MemoryPropertyFlagBits::HOST_VISIBLE_BIT);
            ParticleSsboData * particlesSsboMapped = (ParticleSsboData *)bufferAllocator->MapBuffer(particlesSsbo);

            ResourceCreationContext::DescriptorSetCreateInfo emitterDSCi;
            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor emitterImgDescs[] = {
                {.sampler = sampler, .imageView = emitter.image}};
            ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor emitterBufferDescs[] = {
                {.buffer = emitterUbo.GetBuffer(), .offset = emitterUbo.GetOffset(), .range = emitterUbo.GetSize()},
                {.buffer = particlesSsbo.GetBuffer(),
                 .offset = particlesSsbo.GetOffset(),
                 .range = particlesSsbo.GetSize()}};
            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor emitterDescs[] = {
                {.type = DescriptorType::UNIFORM_BUFFER, .binding = 0, .descriptor = emitterBufferDescs[0]},
                {.type = DescriptorType::STORAGE_BUFFER, .binding = 1, .descriptor = emitterBufferDescs[1]},
                {.type = DescriptorType::COMBINED_IMAGE_SAMPLER, .binding = 2, .descriptor = emitterImgDescs[0]}};
            emitterDSCi.descriptorCount = 3;
            emitterDSCi.descriptors = emitterDescs;
            emitterDSCi.layout = emitterLayout;

            gpuHandles.perFrame.push_back({
                .descriptorSet = ctx.CreateDescriptorSet(emitterDSCi),
                .emitterUbo = emitterUbo,
                .emitterUboMapped = emitterUboMapped,
                .particlesSsbo = particlesSsbo,
                .particlesSsboMapped = particlesSsboMapped,
            });
        }

        sem.Signal();
    });
    sem.Wait();
    emitterGpuHandles.insert({id, gpuHandles});

    return id;
}

std::optional<ParticleEmitter const> ParticleSystem::GetEmitter(ParticleEmitterId id)
{
    auto it = emitters.find(id);
    if (it == emitters.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ParticleSystem::RemoveEmitter(ParticleEmitterId id)
{
    OPTICK_EVENT();
    emitters.erase(id);
    particles.erase(id);
    auto gpuHandlesIt = emitterGpuHandles.find(id);
    if (gpuHandlesIt == emitterGpuHandles.end()) {
        logger.Warn("Did not find GPU handles for particle emitter with id={} when trying to remove it.", id);
    } else {
        Semaphore sem;
        // TODO: Do this in RenderSystem::DestroyResources
        renderer->CreateResources([&sem, this, gpuHandlesIt](ResourceCreationContext & ctx) {
            for (auto & frame : gpuHandlesIt->second.perFrame) {
                bufferAllocator->UnmapBuffer(frame.particlesSsboMapped);
                bufferAllocator->FreeBuffer(frame.particlesSsbo);
                bufferAllocator->UnmapBuffer(frame.emitterUboMapped);
                bufferAllocator->FreeBuffer(frame.emitterUbo);
                ctx.DestroyDescriptorSet(frame.descriptorSet);
            }
            sem.Signal();
        });
        sem.Wait();
        emitterGpuHandles.erase(id);
    }
}
