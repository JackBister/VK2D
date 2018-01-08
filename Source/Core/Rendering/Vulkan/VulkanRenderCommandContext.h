#pragma once
#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Context/RenderContext.h"
#include <vulkan/vulkan.h>

class VulkanRenderCommandContext : public RenderCommandContext
{
	friend class Renderer;
	friend class VulkanCommandContextAllocator;
public:

	VulkanRenderCommandContext(VkCommandBuffer buffer, std::allocator<uint8_t> * allocator) : RenderCommandContext(allocator), buffer(buffer) {}

	virtual ~VulkanRenderCommandContext() { delete allocator; }

	virtual void BeginRecording(RenderCommandContext::InheritanceInfo *) override;
	virtual void EndRecording() override;
	virtual void Reset() override;

	virtual void CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo *pRenderPassBegin, RenderCommandContext::SubpassContents contents) override;
	virtual void CmdBindDescriptorSet(DescriptorSet *) override;
	virtual void CmdBindIndexBuffer(BufferHandle *buffer, size_t offset, RenderCommandContext::IndexType indexType) override;
	virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) override;
	virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) override;
	virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) override;
	virtual void CmdEndRenderPass() override;
	virtual void CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers) override;
	virtual void CmdExecuteCommands(std::vector<RenderCommandContext *>&& commandBuffers) override;
	virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, RenderCommandContext::Rect2D const * pScissors) override;
	virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, RenderCommandContext::Viewport const * pViewports) override;
	virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) override;
protected:
	void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence) override;

	VkCommandBuffer buffer;
};

#endif

