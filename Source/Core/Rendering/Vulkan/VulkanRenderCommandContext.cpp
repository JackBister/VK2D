#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Vulkan/VulkanRenderCommandContext.h"

#include "Core/Rendering/Vulkan/VulkanRenderer.h"

static constexpr VkRect2D ToVulkanRect2D(RenderCommandContext::Rect2D const& rect) {
	return VkRect2D{
		{ rect.offset.x, rect.offset.y },
		{ rect.extent.width, rect.extent.height }
	};
}

void VulkanRenderCommandContext::Execute(Renderer * renderer, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
	std::vector<VkSemaphore> vulkanWaitSems(waitSem.size());
	std::transform(waitSem.begin(), waitSem.end(), vulkanWaitSems.begin(), [](SemaphoreHandle * sem) { return ((VulkanSemaphoreHandle *)sem)->semaphore; });
	std::vector<VkSemaphore> vulkanSignalSems(signalSem.size());
	std::transform(signalSem.begin(), signalSem.end(), vulkanSignalSems.begin(), [](SemaphoreHandle * sem) { return ((VulkanSemaphoreHandle *)sem)->semaphore; });
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->buffer;
	std::vector<VkPipelineStageFlags> flags(vulkanWaitSems.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	submitInfo.pWaitDstStageMask = vulkanWaitSems.size() > 0 ?  &flags[0] : nullptr;
	submitInfo.waitSemaphoreCount = (uint32_t)vulkanWaitSems.size();
	submitInfo.pWaitSemaphores = vulkanWaitSems.size() > 0 ? &vulkanWaitSems[0] : nullptr;
	submitInfo.signalSemaphoreCount = (uint32_t)vulkanSignalSems.size();
	submitInfo.pSignalSemaphores = vulkanSignalSems.size() > 0 ? &vulkanSignalSems[0] : nullptr;
	auto res = vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, signalFence != nullptr ? ((VulkanFenceHandle *)signalFence)->fence : VK_NULL_HANDLE);
	assert(res == VK_SUCCESS);
}

void VulkanRenderCommandContext::BeginRecording(RenderCommandContext::InheritanceInfo * inheritanceInfo)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkCommandBufferInheritanceInfo vkInheritanceInfo = {};
	if (inheritanceInfo != nullptr) {
		vkInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		vkInheritanceInfo.occlusionQueryEnable = VK_FALSE;
		vkInheritanceInfo.subpass = inheritanceInfo->subpass;
		if (inheritanceInfo->framebuffer != nullptr) {
			vkInheritanceInfo.framebuffer = ((VulkanFramebufferHandle *)inheritanceInfo->framebuffer)->framebuffer;
		}
		if (inheritanceInfo->renderPass != nullptr) {
			vkInheritanceInfo.renderPass = ((VulkanRenderPassHandle *)inheritanceInfo->renderPass)->renderPass;
		}
		beginInfo.flags = inheritanceInfo->commandContextUsageFlags;
		beginInfo.pInheritanceInfo = &vkInheritanceInfo;
	}

	vkBeginCommandBuffer(buffer, &beginInfo);
}

void VulkanRenderCommandContext::EndRecording()
{
	vkEndCommandBuffer(buffer);
}

void VulkanRenderCommandContext::Reset()
{
	vkResetCommandBuffer(buffer, 0);
}

void VulkanRenderCommandContext::CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo * pRenderPassBegin, RenderCommandContext::SubpassContents contents)
{
	std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
	std::transform(pRenderPassBegin->pClearValues, &pRenderPassBegin->pClearValues[pRenderPassBegin->clearValueCount], clearValues.begin(), [](ClearValue clearValue) {
		if (clearValue.type == ClearValue::Type::COLOR) {
			return *(VkClearValue *)&clearValue.color;
		} else {
			return *(VkClearValue *)&clearValue.depthStencil;
		}
	});
	VkRenderPassBeginInfo beginInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		((VulkanRenderPassHandle *)pRenderPassBegin->renderPass)->renderPass,
		((VulkanFramebufferHandle *)pRenderPassBegin->framebuffer)->framebuffer,
		ToVulkanRect2D(pRenderPassBegin->renderArea),
		(uint32_t)clearValues.size(),
		&clearValues[0]
	};

	VkSubpassContents subpassContents;
	switch (contents) {
	case RenderCommandContext::SubpassContents::INLINE:
		subpassContents = VK_SUBPASS_CONTENTS_INLINE;
		break;
	case RenderCommandContext::SubpassContents::SECONDARY_COMMAND_BUFFERS:
		subpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
		break;
	}

	vkCmdBeginRenderPass(this->buffer, &beginInfo, subpassContents);
}

void VulkanRenderCommandContext::CmdBindDescriptorSet(DescriptorSet * set)
{
	assert(set != nullptr);
	auto nativeSet = (VulkanDescriptorSet *)set;

	//TODO: compute
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, nativeSet->pipelineLayout, 0, 1, &nativeSet->set, 0, nullptr);
}

void VulkanRenderCommandContext::CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, RenderCommandContext::IndexType indexType)
{
	VkIndexType vkIndexType;
	switch (indexType) {
	case RenderCommandContext::IndexType::UINT16:
		vkIndexType = VK_INDEX_TYPE_UINT16;
		break;
	case RenderCommandContext::IndexType::UINT32:
		vkIndexType = VK_INDEX_TYPE_UINT32;
		break;
	}

	vkCmdBindIndexBuffer(this->buffer, ((VulkanBufferHandle *)buffer)->buffer, offset, vkIndexType);
}

void VulkanRenderCommandContext::CmdBindPipeline(RenderPassHandle::PipelineBindPoint bindPoint, PipelineHandle * pipeline)
{
	VkPipelineBindPoint vkBindPoint;
	switch (bindPoint) {
	case RenderPassHandle::PipelineBindPoint::COMPUTE:
		vkBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		break;
	case RenderPassHandle::PipelineBindPoint::GRAPHICS:
		vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		break;
	}
	vkCmdBindPipeline(this->buffer, vkBindPoint, ((VulkanPipelineHandle *)pipeline)->pipeline);
}

void VulkanRenderCommandContext::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
	vkCmdBindVertexBuffers(this->buffer, binding, 1, &((VulkanBufferHandle *)buffer)->buffer, &offset);
}

void VulkanRenderCommandContext::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset)
{
	vkCmdDrawIndexed(this->buffer, indexCount, instanceCount, firstIndex, vertexOffset, 0);
}

void VulkanRenderCommandContext::CmdEndRenderPass()
{
	vkCmdEndRenderPass(this->buffer);
}

void VulkanRenderCommandContext::CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers)
{
	std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);
	std::transform(pCommandBuffers, &pCommandBuffers[commandBufferCount], commandBuffers.begin(), [](RenderCommandContext * ctx) {
		return ((VulkanRenderCommandContext *)ctx)->buffer;
	});

	vkCmdExecuteCommands(this->buffer, (uint32_t)commandBuffers.size(), &commandBuffers[0]);
}

void VulkanRenderCommandContext::CmdExecuteCommands(std::vector<RenderCommandContext *>&& commandBuffers)
{
	std::vector<VkCommandBuffer> nativeCommandBuffers(commandBuffers.size());
	std::transform(commandBuffers.begin(), commandBuffers.end(), nativeCommandBuffers.begin(), [](RenderCommandContext * ctx) {
		return ((VulkanRenderCommandContext *)ctx)->buffer;
	});

	vkCmdExecuteCommands(this->buffer, (uint32_t)nativeCommandBuffers.size(), &nativeCommandBuffers[0]);
}

void VulkanRenderCommandContext::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, RenderCommandContext::Rect2D const * pScissors)
{
	std::vector<VkRect2D> vkScissors(scissorCount);
	std::transform(pScissors, &pScissors[scissorCount], vkScissors.begin(), ToVulkanRect2D);

	vkCmdSetScissor(this->buffer, firstScissor, scissorCount, &vkScissors[0]);
}

void VulkanRenderCommandContext::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, RenderCommandContext::Viewport const * pViewports)
{
	std::vector<VkViewport> vkViewports(viewportCount);
	std::transform(pViewports, &pViewports[viewportCount], vkViewports.begin(), [](RenderCommandContext::Viewport viewport) {
		return VkViewport{
			viewport.x,
			viewport.y,
			viewport.width,
			viewport.height,
			viewport.minDepth,
			viewport.maxDepth
		};
	});

	vkCmdSetViewport(this->buffer, firstViewport, viewportCount, &vkViewports[0]);
}

void VulkanRenderCommandContext::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData)
{
	vkCmdUpdateBuffer(this->buffer, ((VulkanBufferHandle *)buffer)->buffer, offset, size, pData);
}

#endif
