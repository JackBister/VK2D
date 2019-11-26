#pragma once

#include <cstdint>
#include <functional>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

class IRenderer
{
public:
    virtual uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) = 0;
    virtual std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) = 0;
    virtual Format GetBackbufferFormat() const = 0;
    virtual glm::ivec2 GetResolution() const = 0;
    virtual uint32_t GetSwapCount() const = 0;

    virtual void CreateResources(std::function<void(ResourceCreationContext &)> fun) = 0;
    virtual void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                                      std::vector<SemaphoreHandle *> signalSem,
                                      FenceHandle * signalFence = nullptr) = 0;
    virtual void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) = 0;

    virtual void RecreateSwapchain() = 0;

    virtual void UpdateConfig(RendererConfig) = 0;
};
