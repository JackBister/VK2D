#pragma once
#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Backend/Abstract/CommandBufferAllocator.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

#include <vulkan/vulkan.h>

class VulkanResourceContext : public ResourceCreationContext
{
	friend class Renderer;
public:
	CommandBufferAllocator * CreateCommandBufferAllocator() final override;
	void DestroyCommandBufferAllocator(CommandBufferAllocator *) final override;

	void BufferSubData(BufferHandle *, uint8_t *, size_t offset, size_t size) final override;
	BufferHandle * CreateBuffer(BufferCreateInfo const& ) final override;
	void DestroyBuffer(BufferHandle *) final override;
	uint8_t * MapBuffer(BufferHandle *, size_t, size_t) final override;
	void UnmapBuffer(BufferHandle *) final override;
	ImageHandle * CreateImage(ImageCreateInfo const& ) final override;
	void DestroyImage(ImageHandle *) final override;
	void ImageData(ImageHandle *, std::vector<uint8_t> const&) final override;

	 RenderPassHandle * CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo const& ) final override;
	 void DestroyRenderPass(RenderPassHandle *) final override;

	 ImageViewHandle * CreateImageView(ResourceCreationContext::ImageViewCreateInfo const& ) final override;
	 void DestroyImageView(ImageViewHandle *) final override;

	 FramebufferHandle * CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const& ) final override;
	 void DestroyFramebuffer(FramebufferHandle *) final override;

	 PipelineHandle * CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const& ) final override;
	 void DestroyPipeline(PipelineHandle *) final override;
	 ShaderModuleHandle * CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo const& ) final override;
	 void DestroyShaderModule(ShaderModuleHandle *) final override;

	 SamplerHandle * CreateSampler(ResourceCreationContext::SamplerCreateInfo const& ) final override;
	 void DestroySampler(SamplerHandle *) final override;

	 SemaphoreHandle * CreateSemaphore() final override;
	 void DestroySemaphore(SemaphoreHandle *) final override;

	 DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo const& ) final override;
	 void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *);

	 FenceHandle * CreateFence(bool startSignaled) final override;
	 void DestroyFence(FenceHandle *) final override;

	 VertexInputStateHandle * CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo const& ) final override;
	 void DestroyVertexInputState(VertexInputStateHandle *) final override;

	 DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo const& ) final override;
	 void DestroyDescriptorSet(DescriptorSet *) final override;

private:
	VulkanResourceContext(Renderer * renderer) : renderer(renderer) {}

	Renderer * renderer;
};

#endif
