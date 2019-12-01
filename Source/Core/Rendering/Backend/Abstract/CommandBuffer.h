#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"

class CommandBuffer
{
    friend class Renderer;

public:
    enum class IndexType { UINT16, UINT32 };
    enum class SubpassContents { INLINE, SECONDARY_COMMAND_BUFFERS };
    struct ClearValue {
        enum class Type { COLOR, DEPTH_STENCIL };
        Type type;
        union {
            union {
                float float32[4];
                int32_t int32[4];
                uint32_t uint32[4];
            } color;
            struct {
                float depth;
                uint32_t stencil;
            } depthStencil;
        };
    };
    struct Rect2D {
        struct {
            int32_t x;
            int32_t y;
        } offset;
        struct {
            uint32_t width;
            uint32_t height;
        } extent;
    };
    struct RenderPassBeginInfo {
        RenderPassHandle * renderPass;
        FramebufferHandle * framebuffer;
        Rect2D renderArea;
        uint32_t clearValueCount;
        ClearValue const * pClearValues;
    };
    struct Viewport {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    CommandBuffer(std::allocator<uint8_t> * allocator) : allocator(allocator) {}
    virtual ~CommandBuffer() {}

    struct InheritanceInfo {
        RenderPassHandle * renderPass;
        uint32_t subpass;
        FramebufferHandle * framebuffer;
        uint32_t commandBufferUsageFlags;
    };
    virtual void BeginRecording(InheritanceInfo *) = 0;
    virtual void EndRecording() = 0;
    virtual void Reset() = 0;

    virtual void CmdBeginRenderPass(RenderPassBeginInfo * pRenderPassBegin, SubpassContents contents) = 0;
    virtual void CmdBindDescriptorSets(PipelineLayoutHandle * layout, uint32_t offset,
                                       std::vector<DescriptorSet *>) = 0;
    /*
            OpenGL: glBindBuffer, state tracker saves offset and index type until glDrawElements is issued
    */
    virtual void CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, IndexType indexType) = 0;
    virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) = 0;
    // TODO: Stride not necessary in Vulkan - Can hack around it in OpenGL too
    virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) = 0;
    virtual void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                         uint32_t firstInstance) = 0;
    virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                int32_t vertexOffset) = 0;
    virtual void CmdEndRenderPass() = 0;
    virtual void CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers) = 0;
    // TODO:
    virtual void CmdExecuteCommands(std::vector<CommandBuffer *> && commandBuffers) = 0;
    virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, Rect2D const * pScissors) = 0;
    virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, Viewport const * pViewports) = 0;

    virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) = 0;

protected:
    virtual void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem,
                         FenceHandle * signalFence) = 0;

    // TODO: Stack allocator for throwaway arrays
    std::allocator<uint8_t> * allocator;
};
