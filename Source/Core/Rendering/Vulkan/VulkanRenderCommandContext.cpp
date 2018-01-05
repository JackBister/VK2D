#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Vulkan/VulkanRenderCommandContext.h"

#include "Core/Rendering/Vulkan/VulkanRenderer.h"

static constexpr VkRect2D ToVulkanRect2D(RenderCommandContext::Rect2D const& rect) {
	return VkRect2D{
		{ rect.offset.x, rect.offset.y },
		{ rect.extent.width, rect.extent.height }
	};
}

void VulkanRenderCommandContext::Execute(Renderer * renderer, SemaphoreHandle * waitSem, SemaphoreHandle * signalSem)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->buffer_;
	if (waitSem != nullptr) {
		VkPipelineStageFlags flag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		submitInfo.pWaitDstStageMask = &flag;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &((VulkanSemaphoreHandle *)waitSem)->semaphore_;
	}
	if (signalSem != nullptr) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &((VulkanSemaphoreHandle *)signalSem)->semaphore_;
	}
	auto res = vkQueueSubmit(renderer->vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);
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
			vkInheritanceInfo.framebuffer = ((VulkanFramebufferHandle *)inheritanceInfo->framebuffer)->framebuffer_;
		}
		if (inheritanceInfo->renderPass != nullptr) {
			vkInheritanceInfo.renderPass = ((VulkanRenderPassHandle *)inheritanceInfo->renderPass)->render_pass;
		}
		beginInfo.pInheritanceInfo = &vkInheritanceInfo;
	}
	if (level_ == RenderCommandContextLevel::SECONDARY && inheritanceInfo->renderPass != nullptr) {
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	}

	vkBeginCommandBuffer(buffer_, &beginInfo);
}

void VulkanRenderCommandContext::EndRecording()
{
	vkEndCommandBuffer(buffer_);
}

void VulkanRenderCommandContext::Reset()
{
	vkResetCommandBuffer(buffer_, 0);
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
		((VulkanRenderPassHandle *)pRenderPassBegin->renderPass)->render_pass,
		((VulkanFramebufferHandle *)pRenderPassBegin->framebuffer)->framebuffer_,
		ToVulkanRect2D(pRenderPassBegin->renderArea),
		clearValues.size(),
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

	vkCmdBeginRenderPass(this->buffer_, &beginInfo, subpassContents);

	//TODO: I use this for debuggability in RenderDoc. RenderDoc assumes you will actually swap something before it will show you texture states and such.
	//This allows me to view the state at the beginning of the first frame 
#if 0
	uint32_t imageIndex;
	auto res = vkAcquireNextImageKHR(this->renderer_->vk_device_, renderer_->vk_swapchain_.swapchain, std::numeric_limits<uint64_t>::max(), renderer_->swap_img_available_, VK_NULL_HANDLE, &imageIndex);
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderer_->swap_img_available_;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &renderer_->vk_swapchain_.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	res = vkQueuePresentKHR(renderer_->vk_queue_present_, &presentInfo);
	vkQueueWaitIdle(renderer_->vk_queue_present_);
#endif
}

void VulkanRenderCommandContext::CmdBindDescriptorSet(DescriptorSet * set)
{
	assert(set != nullptr);
	auto nativeSet = (VulkanDescriptorSet *)set;

	//TODO: compute
	vkCmdBindDescriptorSets(buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, nativeSet->pipeline_layout_, 0, 1, &nativeSet->set_, 0, nullptr);
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

	vkCmdBindIndexBuffer(this->buffer_, ((VulkanBufferHandle *)buffer)->buffer_, offset, vkIndexType);
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
	vkCmdBindPipeline(this->buffer_, vkBindPoint, ((VulkanPipelineHandle *)pipeline)->pipeline_);
}

void VulkanRenderCommandContext::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
	vkCmdBindVertexBuffers(this->buffer_, binding, 1, &((VulkanBufferHandle *)buffer)->buffer_, &offset);
}

void VulkanRenderCommandContext::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset)
{
	vkCmdDrawIndexed(this->buffer_, indexCount, instanceCount, firstIndex, vertexOffset, 0);
}

void VulkanRenderCommandContext::CmdEndRenderPass()
{
	vkCmdEndRenderPass(this->buffer_);
}

void VulkanRenderCommandContext::CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers)
{
	std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);
	std::transform(pCommandBuffers, &pCommandBuffers[commandBufferCount], commandBuffers.begin(), [](RenderCommandContext * ctx) {
		return ((VulkanRenderCommandContext *)ctx)->buffer_;
	});

	vkCmdExecuteCommands(this->buffer_, commandBuffers.size(), &commandBuffers[0]);
}

void VulkanRenderCommandContext::CmdExecuteCommands(std::vector<std::unique_ptr<RenderCommandContext>>&& commandBuffers)
{
	std::vector<VkCommandBuffer> nativeCommandBuffers(commandBuffers.size());
	std::transform(commandBuffers.begin(), commandBuffers.end(), nativeCommandBuffers.begin(), [](std::unique_ptr<RenderCommandContext>& ctx) {
		return ((VulkanRenderCommandContext *)ctx.get())->buffer_;
	});

	vkCmdExecuteCommands(this->buffer_, nativeCommandBuffers.size(), &nativeCommandBuffers[0]);
}

void VulkanRenderCommandContext::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, RenderCommandContext::Rect2D const * pScissors)
{
	std::vector<VkRect2D> vkScissors(scissorCount);
	std::transform(pScissors, &pScissors[scissorCount], vkScissors.begin(), ToVulkanRect2D);

	vkCmdSetScissor(this->buffer_, firstScissor, scissorCount, &vkScissors[0]);
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

	vkCmdSetViewport(this->buffer_, firstViewport, viewportCount, &vkViewports[0]);
}

void VulkanRenderCommandContext::CmdSwapWindow()
{
	//TODO
}

void VulkanRenderCommandContext::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData)
{
	vkCmdUpdateBuffer(this->buffer_, ((VulkanBufferHandle *)buffer)->buffer_, offset, size, pData);
}

#endif
