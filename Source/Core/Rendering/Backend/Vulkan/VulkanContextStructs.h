#pragma once
#include <vulkan/vulkan.h>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

struct VulkanBufferHandle : BufferHandle
{
	//TODO: Both necessary? Maybe memory needs its own super-type?
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct VulkanDescriptorSet : DescriptorSet
{
	VkDescriptorSetLayout layout;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet set;
};

struct VulkanDescriptorSetLayoutHandle : DescriptorSetLayoutHandle
{
	VkDescriptorSetLayout layout;
};

struct VulkanFenceHandle : FenceHandle
{
	bool Wait(uint64_t timeOut) final override;

	VkDevice device;
	VkFence fence;
};

struct VulkanFramebufferHandle : FramebufferHandle
{
	VkFramebuffer framebuffer;
};

struct VulkanImageHandle : ImageHandle
{
	VkImage image;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct VulkanImageViewHandle : ImageViewHandle
{
	VkImageView imageView;
};

struct VulkanRenderPassHandle : RenderPassHandle
{
	VkRenderPass renderPass;
};

struct VulkanPipelineHandle : PipelineHandle
{
	VkPipeline pipeline;
};

struct VulkanSamplerHandle : SamplerHandle
{
	VkSampler sampler;
};

struct VulkanShaderModuleHandle : ShaderModuleHandle
{
	VkShaderModule shader;
};

struct VulkanSemaphoreHandle : SemaphoreHandle
{
	VkSemaphore semaphore;
};

struct VulkanVertexInputStateHandle : VertexInputStateHandle
{
	//Saved until the handle is used in the create pipeline call - bad?
	//Should probably be moved into the pipelinecreateinfo as intended, and the OGL renderer will have to adapt instead.
	ResourceCreationContext::VertexInputStateCreateInfo createInfo;
};
