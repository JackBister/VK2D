#pragma once
#include "Core/Rendering/Context/RenderContext.h"

#include <vulkan/vulkan.h>

class VulkanCommandBufferAllocator : CommandContextAllocator
{
public:
	friend class VulkanResourceContext;
	CommandBuffer * CreateContext(CommandBufferCreateInfo const&  createInfo) final override;
	void DestroyContext(CommandBuffer *) final override;
	void Reset() final override;

private:
	VkCommandPool commandPool;
	VkDevice device;
	Renderer * renderer;
};
