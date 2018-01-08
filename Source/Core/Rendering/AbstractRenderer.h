#pragma once

#include <cstdint>

#include <Core/Rendering/Context/RenderContext.h>

class IRenderer
{
public:
	virtual uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) = 0;
	virtual std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) = 0;
	virtual Format GetBackbufferFormat() = 0;
	//virtual FramebufferHandle * GetBackbuffer() = 0;
	virtual uint32_t GetSwapCount() = 0;

	virtual void CreateResources(std::function<void(ResourceCreationContext&)> fun) = 0;
	virtual void ExecuteCommandContext(RenderCommandContext * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem) = 0;
	virtual void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) = 0;
};
