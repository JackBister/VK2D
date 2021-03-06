#ifndef USE_OGL_RENDERER
#include "VulkanCommandBuffer.h"

#include <ThirdParty/optick/src/optick.h>

#include "VulkanCommandBufferAllocator.h"
#include "VulkanContextStructs.h"
#include "VulkanConverterFuncs.h"
#include "VulkanRenderer.h"

void VulkanCommandBuffer::Execute(Renderer * renderer, std::vector<SemaphoreHandle *> waitSem,
                                  std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
    std::vector<VkSemaphore> vulkanWaitSems(waitSem.size());
    std::transform(waitSem.begin(), waitSem.end(), vulkanWaitSems.begin(), [](SemaphoreHandle * sem) {
        return ((VulkanSemaphoreHandle *)sem)->semaphore;
    });
    std::vector<VkSemaphore> vulkanSignalSems(signalSem.size());
    std::transform(signalSem.begin(), signalSem.end(), vulkanSignalSems.begin(), [](SemaphoreHandle * sem) {
        return ((VulkanSemaphoreHandle *)sem)->semaphore;
    });
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->buffer;
    std::vector<VkPipelineStageFlags> flags(vulkanWaitSems.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    submitInfo.pWaitDstStageMask = vulkanWaitSems.size() > 0 ? &flags[0] : nullptr;
    submitInfo.waitSemaphoreCount = (uint32_t)vulkanWaitSems.size();
    submitInfo.pWaitSemaphores = vulkanWaitSems.size() > 0 ? &vulkanWaitSems[0] : nullptr;
    submitInfo.signalSemaphoreCount = (uint32_t)vulkanSignalSems.size();
    submitInfo.pSignalSemaphores = vulkanSignalSems.size() > 0 ? &vulkanSignalSems[0] : nullptr;

    {
        auto queue = renderer->GetGraphicsQueue();
        auto res = vkQueueSubmit(queue.queue,
                                 1,
                                 &submitInfo,
                                 signalFence != nullptr ? ((VulkanFenceHandle *)signalFence)->fence : VK_NULL_HANDLE);
        assert(res == VK_SUCCESS);
    }
}

void VulkanCommandBuffer::BeginRecording(CommandBuffer::InheritanceInfo * inheritanceInfo)
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
        beginInfo.flags = inheritanceInfo->commandBufferUsageFlags;
        beginInfo.pInheritanceInfo = &vkInheritanceInfo;
    }

    vkBeginCommandBuffer(buffer, &beginInfo);
}

void VulkanCommandBuffer::EndRecording()
{
    vkEndCommandBuffer(buffer);
}

void VulkanCommandBuffer::Reset()
{
    vkResetCommandBuffer(buffer, 0);
}

void VulkanCommandBuffer::CmdBeginRenderPass(CommandBuffer::RenderPassBeginInfo * pRenderPassBegin,
                                             CommandBuffer::SubpassContents contents)
{
    std::vector<VkClearValue> clearValues(pRenderPassBegin->clearValueCount);
    std::transform(pRenderPassBegin->pClearValues,
                   &pRenderPassBegin->pClearValues[pRenderPassBegin->clearValueCount],
                   clearValues.begin(),
                   [](ClearValue clearValue) {
                       if (clearValue.type == ClearValue::Type::COLOR) {
                           return *(VkClearValue *)&clearValue.color;
                       } else {
                           return *(VkClearValue *)&clearValue.depthStencil;
                       }
                   });
    VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                    nullptr,
                                    ((VulkanRenderPassHandle *)pRenderPassBegin->renderPass)->renderPass,
                                    ((VulkanFramebufferHandle *)pRenderPassBegin->framebuffer)->framebuffer,
                                    ToVulkanRect2D(pRenderPassBegin->renderArea),
                                    (uint32_t)clearValues.size(),
                                    clearValues.data()};

    VkSubpassContents subpassContents;
    switch (contents) {
    case CommandBuffer::SubpassContents::INLINE:
        subpassContents = VK_SUBPASS_CONTENTS_INLINE;
        break;
    case CommandBuffer::SubpassContents::SECONDARY_COMMAND_BUFFERS:
        subpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
        break;
    default:
        assert(false);
        subpassContents = VK_SUBPASS_CONTENTS_INLINE;
    }

    vkCmdBeginRenderPass(this->buffer, &beginInfo, subpassContents);
}

void VulkanCommandBuffer::CmdBindDescriptorSets(PipelineLayoutHandle * layout, uint32_t offset,
                                                std::vector<DescriptorSet *> sets)
{
    assert(sets.size() > 0);

    auto nativeLayout = ((VulkanPipelineLayoutHandle *)layout)->pipelineLayout;

    std::vector<VkDescriptorSet> nativeSets(sets.size());
    for (size_t i = 0; i < sets.size(); ++i) {
        nativeSets[i] = ((VulkanDescriptorSet *)sets[i])->set;
    }

    // TODO: compute
    vkCmdBindDescriptorSets(
        buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, nativeLayout, offset, nativeSets.size(), &nativeSets[0], 0, nullptr);
}

void VulkanCommandBuffer::CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, CommandBuffer::IndexType indexType)
{
    VkIndexType vkIndexType;
    switch (indexType) {
    case CommandBuffer::IndexType::UINT16:
        vkIndexType = VK_INDEX_TYPE_UINT16;
        break;
    case CommandBuffer::IndexType::UINT32:
        vkIndexType = VK_INDEX_TYPE_UINT32;
        break;
    default:
        assert(false);
        vkIndexType = VK_INDEX_TYPE_UINT32;
    }

    vkCmdBindIndexBuffer(this->buffer, ((VulkanBufferHandle *)buffer)->buffer, offset, vkIndexType);
}

void VulkanCommandBuffer::CmdBindPipeline(RenderPassHandle::PipelineBindPoint bindPoint, PipelineHandle * pipeline)
{
    VkPipelineBindPoint vkBindPoint;
    switch (bindPoint) {
    case RenderPassHandle::PipelineBindPoint::COMPUTE:
        vkBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        break;
    case RenderPassHandle::PipelineBindPoint::GRAPHICS:
        vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        break;
    default:
        assert(false);
        vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
    vkCmdBindPipeline(this->buffer, vkBindPoint, ((VulkanPipelineHandle *)pipeline)->pipeline);
}

void VulkanCommandBuffer::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
    vkCmdBindVertexBuffers(this->buffer, binding, 1, &((VulkanBufferHandle *)buffer)->buffer, &offset);
}

void VulkanCommandBuffer::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                  uint32_t firstInstance)
{
    OPTICK_EVENT();
    vkCmdDraw(this->buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::CmdDrawIndirect(BufferHandle * buffer, size_t offset, uint32_t drawCount)
{
    vkCmdDrawIndirect(
        this->buffer, ((VulkanBufferHandle *)buffer)->buffer, offset, drawCount, sizeof(VkDrawIndirectCommand));
}

void VulkanCommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                         int32_t vertexOffset, uint32_t firstInstance)
{
    OPTICK_EVENT();
    vkCmdDrawIndexed(this->buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::CmdEndRenderPass()
{
    vkCmdEndRenderPass(this->buffer);
}

void VulkanCommandBuffer::CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers)
{
    std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);
    std::transform(pCommandBuffers,
                   &pCommandBuffers[commandBufferCount],
                   commandBuffers.begin(),
                   [](CommandBuffer * ctx) { return ((VulkanCommandBuffer *)ctx)->buffer; });

    vkCmdExecuteCommands(this->buffer, (uint32_t)commandBuffers.size(), &commandBuffers[0]);
}

void VulkanCommandBuffer::CmdExecuteCommands(std::vector<CommandBuffer *> && commandBuffers)
{
    std::vector<VkCommandBuffer> nativeCommandBuffers(commandBuffers.size());
    std::transform(commandBuffers.begin(), commandBuffers.end(), nativeCommandBuffers.begin(), [](CommandBuffer * ctx) {
        return ((VulkanCommandBuffer *)ctx)->buffer;
    });

    vkCmdExecuteCommands(this->buffer, (uint32_t)nativeCommandBuffers.size(), &nativeCommandBuffers[0]);
}

void VulkanCommandBuffer::CmdNextSubpass(SubpassContents contents)
{
    VkSubpassContents subpassContents;
    switch (contents) {
    case CommandBuffer::SubpassContents::INLINE:
        subpassContents = VK_SUBPASS_CONTENTS_INLINE;
        break;
    case CommandBuffer::SubpassContents::SECONDARY_COMMAND_BUFFERS:
        subpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
        break;
    default:
        assert(false);
        subpassContents = VK_SUBPASS_CONTENTS_INLINE;
    }
    vkCmdNextSubpass(this->buffer, subpassContents);
}

void VulkanCommandBuffer::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount,
                                        CommandBuffer::Rect2D const * pScissors)
{
    std::vector<VkRect2D> vkScissors(scissorCount);
    std::transform(pScissors, &pScissors[scissorCount], vkScissors.begin(), ToVulkanRect2D);

    vkCmdSetScissor(this->buffer, firstScissor, scissorCount, &vkScissors[0]);
}

void VulkanCommandBuffer::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount,
                                         CommandBuffer::Viewport const * pViewports)
{
    std::vector<VkViewport> vkViewports(viewportCount);
    std::transform(pViewports, &pViewports[viewportCount], vkViewports.begin(), [](CommandBuffer::Viewport viewport) {
        return VkViewport{
            viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
    });

    vkCmdSetViewport(this->buffer, firstViewport, viewportCount, &vkViewports[0]);
}

void VulkanCommandBuffer::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData)
{
    vkCmdUpdateBuffer(this->buffer, ((VulkanBufferHandle *)buffer)->buffer, offset, size, pData);
}

#endif
