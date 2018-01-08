#pragma once
#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Context/RenderContext.h"

#include <vulkan/vulkan.h>

class VulkanResourceContext : public ResourceCreationContext
{
	friend class Renderer;
public:
	CommandContextAllocator * CreateCommandContextAllocator() final override;
	void DestroyCommandContextAllocator(CommandContextAllocator *) final override;

	void BufferSubData(BufferHandle *, uint8_t *, size_t offset, size_t size) final override;
	BufferHandle * CreateBuffer(BufferCreateInfo) final override;
	void DestroyBuffer(BufferHandle *) final override;
	uint8_t * MapBuffer(BufferHandle *, size_t, size_t) final override;
	void UnmapBuffer(BufferHandle *) final override;
	ImageHandle * CreateImage(ImageCreateInfo) final override;
	void DestroyImage(ImageHandle *) final override;
	void ImageData(ImageHandle *, std::vector<uint8_t> const&) final override;

	 RenderPassHandle * CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo) final override;
	 void DestroyRenderPass(RenderPassHandle *) final override;

	 ImageViewHandle * CreateImageView(ResourceCreationContext::ImageViewCreateInfo) final override;
	 void DestroyImageView(ImageViewHandle *) final override;

	 FramebufferHandle * CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo) final override;
	 void DestroyFramebuffer(FramebufferHandle *) final override;

	 PipelineHandle * CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo) final override;
	 void DestroyPipeline(PipelineHandle *) final override;
	 ShaderModuleHandle * CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo) final override;
	 void DestroyShaderModule(ShaderModuleHandle *) final override;

	 SamplerHandle * CreateSampler(ResourceCreationContext::SamplerCreateInfo) final override;
	 void DestroySampler(SamplerHandle *) final override;

	 SemaphoreHandle * CreateSemaphore() final override;
	 void DestroySemaphore(SemaphoreHandle *) final override;

	 DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo) final override;
	 void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *);

	 FenceHandle * CreateFence(bool startSignaled) final override;
	 void DestroyFence(FenceHandle *) final override;

	 VertexInputStateHandle * CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo) final override;
	 void DestroyVertexInputState(VertexInputStateHandle *) final override;

	 DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo) final override;
	 void DestroyDescriptorSet(DescriptorSet *) final override;

private:
	VulkanResourceContext(Renderer * renderer) : renderer(renderer) {}

	Renderer * renderer;
};


struct VulkanBufferHandle : BufferHandle
{
	//TODO: Both necessary? Maybe memory needs its own super-type?
	VkBuffer buffer;
	VkDeviceMemory memory;
};

class VulkanCommandContextAllocator : CommandContextAllocator
{
public:
	friend class VulkanResourceContext;
	RenderCommandContext * CreateContext(RenderCommandContextCreateInfo * pCreateInfo) final override;
	void DestroyContext(RenderCommandContext *) final override;
	void Reset() final override;

private:
	VkCommandPool commandPool;
	VkDevice device;
	Renderer * renderer;
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
#endif
