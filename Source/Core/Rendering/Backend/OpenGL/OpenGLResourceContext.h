#pragma once
#ifdef USE_OGL_RENDERER

#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

class OpenGLResourceContext : public ResourceCreationContext
{
public:
	void BufferSubData(BufferHandle *, uint8_t *, size_t, size_t) final override;
	BufferHandle * CreateBuffer(BufferCreateInfo const&) final override;
	void DestroyBuffer(BufferHandle *) final override;
	uint8_t * MapBuffer(BufferHandle *, size_t, size_t) final override;
	void UnmapBuffer(BufferHandle *) final override;
	ImageHandle * CreateImage(ImageCreateInfo const&) final override;
	void DestroyImage(ImageHandle *) final override;
	void ImageData(ImageHandle *, std::vector<uint8_t> const&) final override;

	RenderPassHandle * CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo const&) final override;
	void DestroyRenderPass(RenderPassHandle *) final override;

	ImageViewHandle * CreateImageView(ResourceCreationContext::ImageViewCreateInfo const&) final override;
	void DestroyImageView(ImageViewHandle *) final override;

	FramebufferHandle * CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const&) final override;
	void DestroyFramebuffer(FramebufferHandle *) final override;

	PipelineLayoutHandle * CreatePipelineLayout(PipelineLayoutCreateInfo const &) final override;
    void DestroyPipelineLayout(PipelineLayoutHandle *) final override;

	PipelineHandle * CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const&) final override;
	void DestroyPipeline(PipelineHandle *) final override;
	ShaderModuleHandle * CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo const&) final override;
	void DestroyShaderModule(ShaderModuleHandle *) final override;

	SamplerHandle * CreateSampler(ResourceCreationContext::SamplerCreateInfo const&) final override;
	void DestroySampler(SamplerHandle *) final override;

	DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo const&) final override;
	void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *) final override;
	VertexInputStateHandle * CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo &) final override;
	void DestroyVertexInputState(VertexInputStateHandle *) final override;

	DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo const&) final override;
	void DestroyDescriptorSet(DescriptorSet *) final override;

	SemaphoreHandle * CreateSemaphore() final override;
	void DestroySemaphore(SemaphoreHandle *) final override;

	CommandBufferAllocator * CreateCommandBufferAllocator() final override;
	void DestroyCommandBufferAllocator(CommandBufferAllocator *) final override;
	FenceHandle * CreateFence(bool startSignaled) final override;
	void DestroyFence(FenceHandle *) final override;
};

#endif
