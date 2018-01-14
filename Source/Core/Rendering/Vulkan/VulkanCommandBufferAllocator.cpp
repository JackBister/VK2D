#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Vulkan/VulkanCommandBufferAllocator.h"

#include <cassert>

#include "Core/Rendering/Vulkan/VulkanCommandBuffer.h"

CommandBuffer * VulkanCommandBufferAllocator::CreateContext(CommandBufferCreateInfo const& createInfo)
{
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = createInfo.level == RenderCommandContextLevel::PRIMARY ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VkCommandBuffer ret;
	vkAllocateCommandBuffers(device, &allocateInfo, &ret);
	auto retx = new VulkanCommandBuffer(ret, new std::allocator<uint8_t>());
	return retx;
}

void VulkanCommandBufferAllocator::DestroyContext(CommandBuffer * ctx)
{
	assert(ctx != nullptr);
	auto nativeHandle = (VulkanCommandBuffer *)ctx;
	vkFreeCommandBuffers(device, commandPool, 1, &nativeHandle->buffer);
	delete nativeHandle;
}

void VulkanCommandBufferAllocator::Reset()
{
	vkResetCommandPool(device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

#endif
