#pragma once

#include <cstdint>
#include <functional>

#include <Core/Rendering/Context/RenderContext.h>

class IRenderer
{
public:
	virtual uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) = 0;
	virtual std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) = 0;
	virtual Format GetBackbufferFormat() const = 0;
	virtual glm::ivec2 GetResolution() const = 0;
	virtual uint32_t GetSwapCount() const = 0;

	virtual void CreateResources(std::function<void(ResourceCreationContext&)> fun) = 0;
	virtual void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence = nullptr) = 0;
	virtual void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) = 0;
};
