#pragma once
#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Context/RenderContext.h"

#include <atomic>
#include <vulkan/vulkan.h>

class VulkanResourceContext : public ResourceCreationContext
{
	friend class Renderer;
public:
	CommandContextAllocator * CreateCommandContextAllocator() final override;
	void DestroyCommandContextAllocator(CommandContextAllocator *) final override;

	std::unique_ptr<RenderCommandContext> CreateCommandContext(RenderCommandContextCreateInfo * pCreateInfo) final override;
	void DestroyCommandContext(std::unique_ptr<RenderCommandContext>) final override;

	void BufferSubData(BufferHandle *, uint8_t *, size_t offset, size_t size) final override;
	BufferHandle * CreateBuffer(BufferCreateInfo) final override;
	void DestroyBuffer(BufferHandle *) final override;
	uint8_t * MapBuffer(BufferHandle *, size_t, size_t) final override;
	void UnmapBuffer(BufferHandle *) final override;
	ImageHandle * CreateImage(ImageCreateInfo) final override;
	void DestroyImage(ImageHandle *) final override;
	void ImageData(ImageHandle *, std::vector<uint8_t> const&) final override;

	// Inherited via ResourceCreationContext
	virtual RenderPassHandle * CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo);
	virtual void DestroyRenderPass(RenderPassHandle *);

	// Inherited via ResourceCreationContext
	virtual ImageViewHandle * CreateImageView(ResourceCreationContext::ImageViewCreateInfo);
	virtual void DestroyImageView(ImageViewHandle *);

	// Inherited via ResourceCreationContext
	virtual FramebufferHandle * CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo);
	virtual void DestroyFramebuffer(FramebufferHandle *);

	// Inherited via ResourceCreationContext
	virtual PipelineHandle * CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo);
	virtual void DestroyPipeline(PipelineHandle *);
	virtual ShaderModuleHandle * CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo);
	virtual void DestroyShaderModule(ShaderModuleHandle *);

	// Inherited via ResourceCreationContext
	virtual SamplerHandle * CreateSampler(ResourceCreationContext::SamplerCreateInfo);
	virtual void DestroySampler(SamplerHandle *);

	virtual SemaphoreHandle * CreateSemaphore() override;
	virtual void DestroySemaphore(SemaphoreHandle *) override;

	// Inherited via ResourceCreationContext
	virtual DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo) override;
	virtual void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *);

	virtual FenceHandle * CreateFence(bool startSignaled) override;
	virtual void DestroyFence(FenceHandle *) override;

	virtual VertexInputStateHandle * CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo);
	virtual void DestroyVertexInputState(VertexInputStateHandle *);

	virtual DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo) override;
	virtual void DestroyDescriptorSet(DescriptorSet *) override;

private:
	VulkanResourceContext(Renderer * renderer) : renderer(renderer) {}

	Renderer * renderer;
};


struct VulkanBufferHandle : BufferHandle
{
	//TODO: Both necessary? Maybe memory_ needs its own super-type?
	VkBuffer buffer_;
	VkDeviceMemory memory_;
};

struct VulkanCommandContextAllocator : CommandContextAllocator
{
	friend class VulkanResourceContext;
	std::unique_ptr<RenderCommandContext> CreateContext(RenderCommandContextCreateInfo * pCreateInfo) final override;
	void DestroyContext(std::unique_ptr<RenderCommandContext>) final override;
	void Reset() final override;

private:
	VkCommandPool commandPool;
	VkDevice device;
	Renderer * renderer;
};

struct VulkanDescriptorSet : DescriptorSet
{
	VkDescriptorSetLayout layout_;
	VkPipelineLayout pipeline_layout_;
	VkDescriptorSet set_;
};

struct VulkanDescriptorSetLayoutHandle : DescriptorSetLayoutHandle
{
	VkDescriptorSetLayout layout_;
};

struct VulkanFenceHandle : FenceHandle
{
	bool Wait(uint64_t timeOut) final override;

	VkDevice device;
	VkFence fence;

	/*
		TODO:
		Vulkan-specific and hopefully temporary:
		vkSubmitQueue and vkWaitForFence being called on separate threads simultaneously violates the spec for various reasons.
		This means that they must be externally synchronized.
		Unfortunately a mutex doesn't work because it needs to be locked before it even goes to the render queue.
		Instead, the current semantic is: "vkWaitForFences is called, a call to Wait(1) will result in busy waiting until Signal(1) has been called",
		with the idea being that only the VulkanRenderer will downcast FenceHandle to VulkanFenceHandle and gain access to Signal.
		This means that the timeOut parameter to Wait is a little broken, since the extra lock will wait infinitely.
		This should be fixed in the future by removing the RenderQueue concept from the abstract renderer, since Vulkan generally does not seem to need it.
	*/
	void Signal();
private:
	std::atomic_bool locked = false;
};

struct VulkanFramebufferHandle : FramebufferHandle
{
	VkFramebuffer framebuffer_;
};

struct VulkanImageHandle : ImageHandle
{
	VkImage image_;
};

struct VulkanImageViewHandle : ImageViewHandle
{
	VkImageView image_view_;
};

struct VulkanRenderPassHandle : RenderPassHandle
{
	VkRenderPass render_pass;
};

struct VulkanPipelineHandle : PipelineHandle
{
	VkPipeline pipeline_;
};

struct VulkanSamplerHandle : SamplerHandle
{
	VkSampler sampler_;
};

struct VulkanShaderModuleHandle : ShaderModuleHandle
{
	VkShaderModule shader_;
};

struct VulkanSemaphoreHandle : SemaphoreHandle
{
	VkSemaphore semaphore_;
};

struct VulkanVertexInputStateHandle : VertexInputStateHandle
{
	//Saved until the handle is used in the create pipeline call - bad?
	ResourceCreationContext::VertexInputStateCreateInfo create_info_;
};
#endif
