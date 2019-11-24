#pragma once
#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Backend/Abstract/CommandBuffer.h"
#include <vulkan/vulkan.h>

class VulkanCommandBuffer : public CommandBuffer
{
    friend class Renderer;
    friend class VulkanCommandBufferAllocator;

public:
    VulkanCommandBuffer(VkCommandBuffer buffer, std::allocator<uint8_t> * allocator)
        : CommandBuffer(allocator), buffer(buffer)
    {
    }

    virtual ~VulkanCommandBuffer() { delete allocator; }

    virtual void BeginRecording(CommandBuffer::InheritanceInfo *) override;
    virtual void EndRecording() override;
    virtual void Reset() override;

    virtual void CmdBeginRenderPass(CommandBuffer::RenderPassBeginInfo * pRenderPassBegin,
                                    CommandBuffer::SubpassContents contents) override;
    virtual void CmdBindDescriptorSets(PipelineLayoutHandle * layout, uint32_t offset,
                                       std::vector<DescriptorSet *>) override;
    virtual void CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, CommandBuffer::IndexType indexType) override;
    virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) override;
    virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) override;
    virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                int32_t vertexOffset) override;
    virtual void CmdEndRenderPass() override;
    virtual void CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers) override;
    virtual void CmdExecuteCommands(std::vector<CommandBuffer *> && commandBuffers) override;
    virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount,
                               CommandBuffer::Rect2D const * pScissors) override;
    virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount,
                                CommandBuffer::Viewport const * pViewports) override;
    virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) override;

protected:
    void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem,
                 FenceHandle * signalFence) override;

    VkCommandBuffer buffer;
};

#endif
