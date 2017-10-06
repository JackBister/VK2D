#pragma once
#include "Core/Rendering/Context/RenderContext.h"

#include <vulkan/vulkan.h>

class VulkanRenderCommandContext : public RenderCommandContext
{
	friend class Renderer;
public:

	VulkanRenderCommandContext() : RenderCommandContext(new std::allocator<uint8_t>) {}
	VulkanRenderCommandContext(std::allocator<uint8_t> * allocator) : RenderCommandContext(allocator) {}

	virtual ~VulkanRenderCommandContext() { delete allocator; }

	virtual void CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo *pRenderPassBegin, RenderCommandContext::SubpassContents contents) override;
	virtual void CmdBindDescriptorSet(DescriptorSet *) override;
	virtual void CmdBindIndexBuffer(BufferHandle *buffer, size_t offset, RenderCommandContext::IndexType indexType) override;
	virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) override;
	virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) override;
	virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) override;
	virtual void CmdEndRenderPass() override;
	virtual void CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers) override;
	virtual void CmdExecuteCommands(std::vector<std::unique_ptr<RenderCommandContext>>&& commandBuffers) override;
	virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, RenderCommandContext::Rect2D const * pScissors) override;
	virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, RenderCommandContext::Viewport const * pViewports) override;
	virtual void CmdSwapWindow() override;
	virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) override;

protected:
	void Execute(Renderer *) override;
private:
	VkCommandBuffer buffer_;
};

class VulkanResourceContext : public ResourceCreationContext
{
public:
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

	// Inherited via ResourceCreationContext
	virtual DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo) override;
	virtual void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *);
	virtual VertexInputStateHandle * CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo);
	virtual void DestroyVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo *);

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

struct VulkanDescriptorSet : DescriptorSet
{
	VkDescriptorSet set_;
};

struct VulkanDescriptorSetLayoutHandle : DescriptorSetLayoutHandle
{
	VkDescriptorSetLayout layout_;
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

struct VulkanVertexInputStateHandle : VertexInputStateHandle
{
	//Saved until the handle is used in the create pipeline call - bad?
	ResourceCreationContext::VertexInputStateCreateInfo create_info_;
};
