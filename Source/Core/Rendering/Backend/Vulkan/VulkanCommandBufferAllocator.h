#pragma once
#include "Core/Rendering/Backend/Abstract/CommandBufferAllocator.h"

#include <vulkan/vulkan.h>

class VulkanCommandBufferAllocator : CommandBufferAllocator
{
public:
    friend class VulkanResourceContext;
    CommandBuffer * CreateBuffer(CommandBufferCreateInfo const & createInfo) final override;
    void DestroyContext(CommandBuffer *) final override;
    void Reset() final override;

private:
    VkCommandPool commandPool;
    VkDevice device;
    Renderer * renderer;
};
