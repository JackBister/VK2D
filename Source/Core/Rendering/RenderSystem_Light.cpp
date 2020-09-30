#include "RenderSystem.h"

#include <optick/optick.h>

#include "Core/FrameContext.h"

constexpr size_t MAX_LIGHTS = 512;

LightInstanceId RenderSystem::CreatePointLightInstance(bool isActive, glm::vec3 color)
{
    OPTICK_EVENT();
    lights.emplace_back();
    auto id = lights.size() - 1;
    auto & instance = lights[id];
    instance.id = id;
    instance.type = LightType::POINT;
    instance.color = color;
    instance.isActive = isActive;
    return id;
}

void RenderSystem::DestroyLightInstance(LightInstanceId id)
{
    OPTICK_EVENT();
    GetLight(id)->isActive = false;
    // TODO:
}

LightInstance * RenderSystem::GetLight(LightInstanceId id)
{
    return &lights[id];
}

void RenderSystem::PreRenderLights(std::vector<UpdateLight> const & updates)
{
    OPTICK_EVENT();
    for (auto const & update : updates) {
        auto light = GetLight(update.lightInstance);
        light->isActive = update.isActive;
        light->localToWorld = update.localToWorld;
        light->color = update.color;
    }
}

void RenderSystem::UpdateLights(FrameContext & context)
{
    OPTICK_EVENT();
    if (lights.size() == 0) {
        return;
    }

    auto & currFrame = frameInfo[context.currentGpuFrameIndex];

    if (currFrame.lightsMapped == nullptr) {
        Semaphore sem;
        renderer->CreateResources([this, &sem, &currFrame](ResourceCreationContext & ctx) {
            if (currFrame.lights) {
                ctx.UnmapBuffer(currFrame.lights);
                ctx.DestroyBuffer(currFrame.lights);
                ctx.DestroyDescriptorSet(currFrame.lightsDescriptorSet);
            }
            currFrame.lightsSize = MAX_LIGHTS * sizeof(LightGpuData);
            {
                ResourceCreationContext::BufferCreateInfo ci;
                ci.size = currFrame.lightsSize;
                ci.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                ci.usage = BufferUsageFlags::UNIFORM_BUFFER_BIT;
                currFrame.lights = ctx.CreateBuffer(ci);
            }
            {
                ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
                    currFrame.lights, 0, currFrame.lightsSize};
                ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptor[1] = {
                    {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};
                ResourceCreationContext::DescriptorSetCreateInfo ci;
                ci.descriptorCount = 1;
                ci.descriptors = descriptor;
                ci.layout = this->lightsLayout;
                currFrame.lightsDescriptorSet = ctx.CreateDescriptorSet(ci);
            }

            currFrame.lightsMapped = (LightGpuData *)ctx.MapBuffer(currFrame.lights, 0, currFrame.lightsSize);

            sem.Signal();
        });
        sem.Wait();
    }

    for (size_t i = 0; i < MAX_LIGHTS; ++i) {
        if (lights.size() <= i) {
            currFrame.lightsMapped[i].isActive = 0;
        } else {
            auto const & light = lights[i];
            currFrame.lightsMapped[i].isActive = light.isActive ? 1 : 0;
            currFrame.lightsMapped[i].color = light.color;
            currFrame.lightsMapped[i].localToWorld = light.localToWorld;
        }
    }
}
