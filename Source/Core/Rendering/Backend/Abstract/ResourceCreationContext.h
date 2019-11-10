#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "Core/Rendering/Backend/Abstract/CommandBufferAllocator.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"


class ResourceCreationContext
{
public:
	virtual CommandBufferAllocator * CreateCommandBufferAllocator() = 0;
	virtual void DestroyCommandBufferAllocator(CommandBufferAllocator *) = 0;

	virtual void BufferSubData(BufferHandle *, uint8_t *, size_t offset, size_t size) = 0;
	/*
		OpenGL: glGenBuffers + glBufferData
		Vulkan: vkCreateBuffer (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) + vkAllocateMemory + vkBindBufferMemory
		OpenGL: glDeleteBuffers
		Vulkan: vkFreeMemory + vkDestroyBuffer
	*/
	struct BufferCreateInfo
	{
		size_t size;
		uint32_t usage;
		MemoryPropertyFlagBits memoryProperties;
	};
	virtual BufferHandle * CreateBuffer(BufferCreateInfo const&) = 0;
	virtual void DestroyBuffer(BufferHandle *) = 0;
	/*
		OpenGL: glMapBuffer
		Vulkan: vkMapMemory -- Vulkan handle needs to track both buffer and memory handle?
		OpenGL: glUnmapBuffer
		Vulkan: vkUnmapMemory
	*/
	virtual uint8_t * MapBuffer(BufferHandle *, size_t offset, size_t size) = 0;
	virtual void UnmapBuffer(BufferHandle *) = 0;

	struct DescriptorSetCreateInfo
	{
		struct BufferDescriptor
		{
			BufferHandle * buffer;
			size_t offset;
			size_t range;
		};

		struct ImageDescriptor
		{
			SamplerHandle * sampler;
			ImageViewHandle * imageView;
		};

		struct Descriptor
		{
			DescriptorType type;
			uint32_t binding;
			std::variant<BufferDescriptor, ImageDescriptor> descriptor;
		};

		size_t descriptorCount;
		Descriptor * descriptors;
		//TODO: optional
		DescriptorSetLayoutHandle * layout;
	};
	virtual DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo const&) = 0;
	virtual void DestroyDescriptorSet(DescriptorSet *) = 0;

	/*
		TODO: Diverges a bit from Vulkan
	*/
	struct DescriptorSetLayoutCreateInfo
	{
		struct Binding
		{
			uint32_t binding;
			DescriptorType descriptorType;
			uint32_t stageFlags;
		};
		uint32_t bindingCount;
		Binding const * pBinding;
	};
	virtual DescriptorSetLayoutHandle * CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo const&) = 0;
	virtual void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle *) = 0;	

	virtual FenceHandle * CreateFence(bool startSignaled) = 0;
	virtual void DestroyFence(FenceHandle *) = 0;

	struct FramebufferCreateInfo
	{
		RenderPassHandle * renderPass;
		uint32_t attachmentCount;
		ImageViewHandle const ** pAttachments;
		uint32_t width, height, layers;
	};
	virtual FramebufferHandle * CreateFramebuffer(FramebufferCreateInfo const&) = 0;
	virtual void DestroyFramebuffer(FramebufferHandle *) = 0;
	/*
		OpenGL: glGenTextures + glTexStorage
		Vulkan: vkCreateImage + vkAllocateMemory + vkBindImageMemory
	*/
	struct ImageCreateInfo
	{
		Format format;
		ImageHandle::Type type;
		uint32_t width, height, depth;
		uint32_t mipLevels;
		uint32_t usage;
	};
	virtual ImageHandle * CreateImage(ImageCreateInfo const&) = 0;
	virtual void DestroyImage(ImageHandle *) = 0;
	/*
		OpenGL: glTexSubImage
		Vulkan: vkMapMemory + memcpy + vkUnmapMemory
	*/
	virtual void ImageData(ImageHandle *, std::vector<uint8_t> const&) = 0;

	struct ImageViewCreateInfo
	{
		ImageHandle * image;
		ImageViewHandle::Type viewType;
		Format format;
		ImageViewHandle::ComponentMapping components;
		ImageViewHandle::ImageSubresourceRange subresourceRange;
	};
	virtual ImageViewHandle * CreateImageView(ImageViewCreateInfo const&) = 0;
	virtual void DestroyImageView(ImageViewHandle *) = 0;

	struct GraphicsPipelineCreateInfo
	{
		struct PipelineShaderStageCreateInfo {
			uint32_t stage;
			ShaderModuleHandle * module;
			std::string name;
		};

		struct PipelineRasterizationStateCreateInfo {
			CullMode cullMode;
			FrontFace frontFace;
		};
	
		uint32_t stageCount;
		PipelineShaderStageCreateInfo * pStages;
		VertexInputStateHandle * vertexInputState;
		PipelineRasterizationStateCreateInfo * rasterizationState;
		DescriptorSetLayoutHandle * descriptorSetLayout;
		RenderPassHandle * renderPass;
		uint32_t subpass;
	};
	virtual PipelineHandle * CreateGraphicsPipeline(GraphicsPipelineCreateInfo const&) = 0;
	virtual void DestroyPipeline(PipelineHandle *) = 0;

	struct RenderPassCreateInfo
	{
		uint32_t attachmentCount;
		RenderPassHandle::AttachmentDescription const * pAttachments;
		uint32_t subpassCount;
		RenderPassHandle::SubpassDescription const * pSubpasses;
		uint32_t dependencyCount;
		RenderPassHandle::SubpassDependency const * pDependencies;
	};
	virtual RenderPassHandle * CreateRenderPass(RenderPassCreateInfo const&) = 0;
	virtual void DestroyRenderPass(RenderPassHandle *) = 0;

	struct SamplerCreateInfo
	{
		Filter magFilter;
		Filter minFilter;
		AddressMode addressModeU, addressModeV;
	};
	virtual SamplerHandle * CreateSampler(SamplerCreateInfo const&) = 0;
	virtual void DestroySampler(SamplerHandle *) = 0;

	virtual SemaphoreHandle * CreateSemaphore() = 0;
	virtual void DestroySemaphore(SemaphoreHandle *) = 0;

	struct ShaderModuleCreateInfo
	{
		enum class Type
		{
			VERTEX_SHADER,
			FRAGMENT_SHADER
		};
		Type type;
		size_t codeSize;
		uint8_t const * pCode;
	};
	virtual ShaderModuleHandle * CreateShaderModule(ShaderModuleCreateInfo const&) = 0;
	virtual void DestroyShaderModule(ShaderModuleHandle *) = 0;

	struct VertexInputStateCreateInfo
	{
		//TODO: Use Format instead of Type + size + normalized
		struct VertexAttributeDescription
		{
			uint32_t binding;
			uint32_t location;
			VertexComponentType type;
			uint8_t size;
			bool normalized;
			uint32_t offset;
		};
		struct VertexBindingDescription
		{
			uint32_t binding;
			uint32_t stride;
		};

		uint32_t vertexBindingDescriptionCount;
		VertexBindingDescription const * pVertexBindingDescriptions;
		uint32_t vertexAttributeDescriptionCount;
		VertexAttributeDescription const * pVertexAttributeDescriptions;
	};
	virtual VertexInputStateHandle * CreateVertexInputState(VertexInputStateCreateInfo const&) = 0;
	virtual void DestroyVertexInputState(VertexInputStateHandle *) = 0;

protected:
	std::allocator<uint8_t> allocator;	
};
