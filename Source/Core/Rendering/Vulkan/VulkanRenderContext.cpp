#include "Core/Rendering/Vulkan/VulkanRenderContext.h"

#include "Core/Rendering/Vulkan/VulkanRenderer.h"

static constexpr VkComponentSwizzle ToVulkanComponentSwizzle(ComponentSwizzle swizzle) {
	switch (swizzle)
	{
	case ComponentSwizzle::IDENTITY:
		return VK_COMPONENT_SWIZZLE_IDENTITY;
	case ComponentSwizzle::ZERO:
		return VK_COMPONENT_SWIZZLE_ZERO;
	case ComponentSwizzle::ONE:
		return VK_COMPONENT_SWIZZLE_ONE;
	case ComponentSwizzle::R:
		return VK_COMPONENT_SWIZZLE_R;
	case ComponentSwizzle::G:
		return VK_COMPONENT_SWIZZLE_G;
	case ComponentSwizzle::B:
		return VK_COMPONENT_SWIZZLE_B;
	case ComponentSwizzle::A:
		return VK_COMPONENT_SWIZZLE_A;
	default:
		return VK_COMPONENT_SWIZZLE_IDENTITY;
	}
}

static constexpr VkComponentMapping ToVulkanComponentMapping(ImageViewHandle::ComponentMapping mapping) {
	return VkComponentMapping{
		ToVulkanComponentSwizzle(mapping.r),
		ToVulkanComponentSwizzle(mapping.g),
		ToVulkanComponentSwizzle(mapping.b),
		ToVulkanComponentSwizzle(mapping.a),
	};
}

static constexpr VkDescriptorType ToVulkanDescriptorType(DescriptorType descriptorType) {
	switch (descriptorType) {
	case DescriptorType::SAMPLER:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	case DescriptorType::UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	return VK_DESCRIPTOR_TYPE_SAMPLER;
}

static constexpr VkImageSubresourceRange ToVulkanSubResourceRange(ImageViewHandle::ImageSubresourceRange range) {
	return VkImageSubresourceRange{
		range.aspectMask,
		range.baseMipLevel,
		range.levelCount,
		range.baseArrayLayer,
		range.layerCount
	};
}

static VkFormat ToVulkanFormat(Format const& format) {
	switch (format) {
	case Format::R8:
		return VkFormat::VK_FORMAT_R8_UINT;
	case Format::RG8:
		return VkFormat::VK_FORMAT_R8G8_UINT;
	case Format::RGB8:
		return VkFormat::VK_FORMAT_R8G8B8_UINT;
	case Format::RGBA8:
		return VkFormat::VK_FORMAT_R8G8B8A8_UINT;
	}
}

BufferHandle * VulkanResourceContext::CreateBuffer(BufferCreateInfo ci)
{
	VkMemoryAllocateInfo info{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		ci.size,
		//TODO: memory type
		0
	};

	auto const ret = (VulkanBufferHandle *)allocator.allocate(sizeof(VulkanBufferHandle));
	vkAllocateMemory(renderer->vk_device_, &info, nullptr, &ret->memory_);
	return ret;
}

void VulkanResourceContext::DestroyBuffer(BufferHandle * buffer)
{
	assert(buffer != nullptr);

	vkFreeMemory(renderer->vk_device_, ((VulkanBufferHandle *)buffer)->memory_, nullptr);
}

uint8_t * VulkanResourceContext::MapBuffer(BufferHandle * buffer, size_t offset, size_t size)
{
	assert(buffer != nullptr);

	uint8_t * ret;
	vkMapMemory(renderer->vk_device_, ((VulkanBufferHandle *)buffer)->memory_, offset, size, 0, (void **)&ret);
	return ret;
}

void VulkanResourceContext::UnmapBuffer(BufferHandle * buffer)
{
	assert(buffer != nullptr);

	vkUnmapMemory(renderer->vk_device_, ((VulkanBufferHandle *)buffer)->memory_);
}

ImageHandle * VulkanResourceContext::CreateImage(ImageCreateInfo ci)
{
	VkImageType const imageType = [](ImageHandle::Type type) {
		switch (type) {
		case ImageHandle::Type::TYPE_1D:
			return VkImageType::VK_IMAGE_TYPE_1D;
		case ImageHandle::Type::TYPE_2D:
			return VkImageType::VK_IMAGE_TYPE_2D;
		}
	}(ci.type);

	VkFormat const format = ToVulkanFormat(ci.format);

	VkExtent3D const extent{
		ci.width,
		ci.height,
		ci.depth
	};

	VkImageCreateInfo info{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		imageType,
		format,
		extent,
		ci.mipLevels,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		ci.usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_GENERAL
	};

	auto const ret = (VulkanImageHandle *)allocator.allocate(sizeof(VulkanImageHandle));
	vkCreateImage(renderer->vk_device_, &info, nullptr, &ret->image_);
	return ret;
}

void VulkanResourceContext::DestroyImage(ImageHandle * img)
{
	assert(img != nullptr);

	vkDestroyImage(renderer->vk_device_, ((VulkanImageHandle *)img)->image_, nullptr);
}

void VulkanResourceContext::ImageData(ImageHandle * img, std::vector<uint8_t> const& data)
{
	assert(img != nullptr);

	auto const nativeImg = (VulkanImageHandle *)img;

	VkMemoryRequirements const memoryRequirements = [this](VkImage img) {
		VkMemoryRequirements ret;
		vkGetImageMemoryRequirements(renderer->vk_device_, img, &ret);
		return ret;
	}(nativeImg->image_);

	assert(data.size() <= memoryRequirements.size);

	VkMemoryAllocateInfo const info{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		memoryRequirements.size,
		//TODO: memory type
		memoryRequirements.memoryTypeBits
	};

	VkDeviceMemory const memory = [this](VkMemoryAllocateInfo const& info) {
		VkDeviceMemory ret;
		vkAllocateMemory(renderer->vk_device_, &info, nullptr, &ret);
		return ret;
	}(info);

	uint8_t * const mappedMemory = [this](VkDeviceMemory const& memory, size_t size) {
		uint8_t * ret = nullptr;
		vkMapMemory(renderer->vk_device_, memory, 0, size, 0, (void **)&ret);
		return ret;
	}(memory, memoryRequirements.size);

	for (size_t i = 0; i < data.size(); ++i) {
		mappedMemory[i] = data[i];
	}

	vkUnmapMemory(renderer->vk_device_, memory);

	vkBindImageMemory(renderer->vk_device_, nativeImg->image_, memory, 0);
}

RenderPassHandle * VulkanResourceContext::CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo ci)
{
	//TODO
	std::vector<VkAttachmentDescription> attachments;
#if 0
	if (ci.attachmentCount > 0) {
		attachments.reserve(ci.attachmentCount);
		std::transform(ci.pAttachments, &ci.pAttachments[ci.attachmentCount - 1], attachments.begin(), [](RenderPassHandle::AttachmentDescription description) {
		});
	}
#endif
	std::vector<VkSubpassDescription> subpassDescriptions;
#if 0
	if (ci.subpassCount > 0) {
		subpassDescriptions.reserve(ci.subpassCount);
		std::transform(ci.pSubpasses, &ci.pSubpasses[ci.subpassCount - 1], subpassDescriptions.begin(), [](RenderPassHandle::SubpassDescription description) {
		});
	}
#endif
	std::vector<VkSubpassDependency> subpassDependencies;
#if 0
	if (ci.dependencyCount > 0) {
		subpassDependencies.reserve(ci.dependencyCount);
		std::transform(ci.pDependencies, &ci.pDependencies[ci.dependencyCount - 1], subpassDependencies.begin(), [](RenderPassHandle::SubpassDependency dependency) {
		});
	}
#endif
	VkRenderPassCreateInfo const info{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		ci.attachmentCount,
		&attachments[0],
		ci.subpassCount,
		&subpassDescriptions[0],
		ci.dependencyCount,
		&subpassDependencies[0]
	};

	auto const ret = (VulkanRenderPassHandle *)allocator.allocate(sizeof(VulkanRenderPassHandle));
	vkCreateRenderPass(renderer->vk_device_, &info, nullptr, &ret->render_pass);
	return ret;
}

void VulkanResourceContext::DestroyRenderPass(RenderPassHandle * pass)
{
	assert(pass != nullptr);

	vkDestroyRenderPass(renderer->vk_device_, ((VulkanRenderPassHandle *)pass)->render_pass, nullptr);
}

ImageViewHandle * VulkanResourceContext::CreateImageView(ResourceCreationContext::ImageViewCreateInfo const ci)
{
	VkImageViewType const type = [](ImageViewHandle::Type type) {
		switch (type)
		{
		case ImageViewHandle::Type::TYPE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case ImageViewHandle::Type::TYPE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case ImageViewHandle::Type::TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		case ImageViewHandle::Type::TYPE_CUBE:
			return VK_IMAGE_VIEW_TYPE_CUBE;
		case ImageViewHandle::Type::TYPE_1D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		case ImageViewHandle::Type::TYPE_2D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case ImageViewHandle::Type::CUBE_ARRAY:
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		default:
			printf("Unknown ImageViewType %d\n", type);
			assert(false);
			return VK_IMAGE_VIEW_TYPE_1D;
		}
	}(ci.viewType);

	VkImageViewCreateInfo info{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		((VulkanImageHandle *)ci.image)->image_,
		type,
		ToVulkanFormat(ci.format),
		ToVulkanComponentMapping(ci.components),
		ToVulkanSubResourceRange(ci.subresourceRange)
	};

	auto ret = (VulkanImageViewHandle *)allocator.allocate(sizeof(VulkanImageViewHandle));
	vkCreateImageView(renderer->vk_device_, &info, nullptr, &ret->image_view_);
	ret->image = ci.image;

	return ret;
}

void VulkanResourceContext::DestroyImageView(ImageViewHandle * view)
{
	assert(view != nullptr);

	vkDestroyImageView(renderer->vk_device_, ((VulkanImageViewHandle *)view)->image_view_, nullptr);
}

FramebufferHandle * VulkanResourceContext::CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const ci)
{
	VkFramebufferCreateInfo const info{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		nullptr,
		0,
		((VulkanRenderPassHandle *)ci.renderPass)->render_pass,
		ci.attachmentCount,
		//TODO
		nullptr,
		ci.width,
		ci.height,
		ci.layers
	};

	auto ret = (VulkanFramebufferHandle *)allocator.allocate(sizeof(VulkanFramebufferHandle));
	vkCreateFramebuffer(renderer->vk_device_, &info, nullptr, &ret->framebuffer_);
	return ret;
}

void VulkanResourceContext::DestroyFramebuffer(FramebufferHandle * fb)
{
	assert(fb != nullptr);

	vkDestroyFramebuffer(renderer->vk_device_, ((VulkanFramebufferHandle *)fb)->framebuffer_, nullptr);
}

PipelineHandle * VulkanResourceContext::CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const ci)
{
	VkGraphicsPipelineCreateInfo const info{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		ci.stageCount,
		//TODO:

	};
	return nullptr;
}

void VulkanResourceContext::DestroyPipeline(PipelineHandle * pipeline)
{
	assert(pipeline != nullptr);

	vkDestroyPipeline(renderer->vk_device_, ((VulkanPipelineHandle *)pipeline)->pipeline_, nullptr);
}

ShaderModuleHandle * VulkanResourceContext::CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo ci)
{
	//TODO: glslang
	VkShaderModuleCreateInfo info{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		ci.codeSize,

	};

	auto ret = (VulkanShaderModuleHandle *)allocator.allocate(sizeof(ShaderModuleHandle *));
	vkCreateShaderModule(renderer->vk_device_, &info, nullptr, &ret->shader_);
	return ret;
}

void VulkanResourceContext::DestroyShaderModule(ShaderModuleHandle * shader)
{
	assert(shader != nullptr);

	vkDestroyShaderModule(renderer->vk_device_, ((VulkanShaderModuleHandle *)shader)->shader_, nullptr);
}

SamplerHandle * VulkanResourceContext::CreateSampler(ResourceCreationContext::SamplerCreateInfo ci)
{
	VkSamplerCreateInfo info{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,

	};
	return nullptr;
}

void VulkanResourceContext::DestroySampler(SamplerHandle * sampler)
{
	assert(sampler != nullptr);
	vkDestroySampler(renderer->vk_device_, ((VulkanSamplerHandle *)sampler)->sampler_, nullptr);
}

DescriptorSetLayoutHandle * VulkanResourceContext::CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo info)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(info.bindingCount);
	for (uint32_t i = 0; i < info.bindingCount; ++i) {
		bindings.push_back(VkDescriptorSetLayoutBinding{
			info.pBinding[i].binding,
			ToVulkanDescriptorType(info.pBinding[i].descriptorType),
			1,
			info.pBinding[i].stageFlags,
			nullptr
		});
	}
	VkDescriptorSetLayoutCreateInfo ci{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		info.bindingCount,
		&bindings[0]
	};
	auto ret = (VulkanDescriptorSetLayoutHandle *)allocator.allocate(sizeof(VulkanDescriptorSetLayoutHandle));
	auto res = vkCreateDescriptorSetLayout(renderer->vk_device_, &ci, nullptr, &(ret->layout_));
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanRenderCommandContext::Execute(Renderer * renderer)
{
}
