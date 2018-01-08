#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Vulkan/VulkanRenderContext.h"

#include "Core/Rendering/Vulkan/VulkanRenderer.h"

static constexpr VkImageLayout ToVulkanImageLayout(ImageLayout layout);

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

static constexpr VkSamplerAddressMode ToVulkanAddressMode(AddressMode mode) {
	switch (mode) {
	case AddressMode::REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case AddressMode::MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case AddressMode::CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case AddressMode::CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case AddressMode::MIRROR_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		assert(false);
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

static constexpr VkAttachmentReference ToVulkanAttachmentReference(RenderPassHandle::AttachmentReference const reference) {
	return VkAttachmentReference{
		reference.attachment,
		ToVulkanImageLayout(reference.layout)
	};
}

static constexpr uint32_t ToVulkanBufferUsageFlagBits(BufferUsageFlags flags) {
	return (uint32_t)flags;
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
	case DescriptorType::COMBINED_IMAGE_SAMPLER:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescriptorType::UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	return VK_DESCRIPTOR_TYPE_SAMPLER;
}

static constexpr VkFilter ToVulkanFilter(Filter filter) {
	switch (filter) {
	case Filter::NEAREST:
		return VK_FILTER_NEAREST;
	case Filter::LINEAR:
		return VK_FILTER_LINEAR;
	default:
		assert(false);
		return VK_FILTER_LINEAR;
	}
}

static constexpr VkImageLayout ToVulkanImageLayout(ImageLayout layout) {
	switch (layout) {
	case ImageLayout::UNDEFINED:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	case ImageLayout::GENERAL:
		return VK_IMAGE_LAYOUT_GENERAL;
	case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case ImageLayout::TRANSFER_SRC_OPTIMAL:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case ImageLayout::TRANSFER_DST_OPTIMAL:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case ImageLayout::PREINITIALIZED:
		return VK_IMAGE_LAYOUT_PREINITIALIZED;
	case ImageLayout::PRESENT_SRC_KHR:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	printf("Unknown image layout %d\n", layout);
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static constexpr VkAttachmentLoadOp ToVulkanLoadOp(RenderPassHandle::AttachmentDescription::LoadOp op) {
	switch (op) {
	case RenderPassHandle::AttachmentDescription::LoadOp::CLEAR:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case RenderPassHandle::AttachmentDescription::LoadOp::LOAD:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	default:
		assert(false);
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}

static constexpr VkPipelineBindPoint ToVulkanPipelineBindPoint(RenderPassHandle::PipelineBindPoint point) {
	switch (point) {
	case RenderPassHandle::PipelineBindPoint::GRAPHICS:
		return VK_PIPELINE_BIND_POINT_GRAPHICS;
	case RenderPassHandle::PipelineBindPoint::COMPUTE:
		return VK_PIPELINE_BIND_POINT_COMPUTE;
	default:
		assert(false);
		return VK_PIPELINE_BIND_POINT_GRAPHICS;
	}
}

static constexpr VkShaderStageFlagBits ToVulkanShaderStage(uint32_t stage) {
	return (VkShaderStageFlagBits)stage;
}

static constexpr VkAttachmentStoreOp ToVulkanStoreOp(RenderPassHandle::AttachmentDescription::StoreOp op) {
	switch (op) {
	case RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case RenderPassHandle::AttachmentDescription::StoreOp::STORE:
		return VK_ATTACHMENT_STORE_OP_STORE;
	default:
		assert(false);
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
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
	case Format::B8G8R8A8_UNORM:
		return VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	case Format::R8:
		return VkFormat::VK_FORMAT_R8_UNORM;
	case Format::RG8:
		return VkFormat::VK_FORMAT_R8G8_UNORM;
	case Format::RGB8:
		return VkFormat::VK_FORMAT_R8G8B8_UNORM;
	case Format::RGBA8:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	default:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	}
}

CommandContextAllocator * VulkanResourceContext::CreateCommandContextAllocator()
{
	auto mem = allocator.allocate(sizeof(VulkanCommandContextAllocator));
	auto ret = new (mem) VulkanCommandContextAllocator();
	ret->device = renderer->basics.device;
	ret->renderer = renderer;
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = renderer->graphicsQueueIdx;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(renderer->basics.device, &createInfo, nullptr, &ret->commandPool);
	return ret;
}

void VulkanResourceContext::DestroyCommandContextAllocator(CommandContextAllocator * pool)
{
	assert(pool != nullptr);
	auto nativeHandle = (VulkanCommandContextAllocator *)pool;
	vkDestroyCommandPool(renderer->basics.device, nativeHandle->commandPool, nullptr);
}

void VulkanResourceContext::BufferSubData(BufferHandle * buffer, uint8_t * data, size_t offset, size_t size)
{
	auto const nativeHandle = (VulkanBufferHandle *)buffer;

	VkBufferCreateInfo stagingInfo = {};
	stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingInfo.size = size;
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer stagingBuffer;
	auto res = vkCreateBuffer(renderer->basics.device, &stagingInfo, nullptr, &stagingBuffer);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements stagingRequirements = {};
	vkGetBufferMemoryRequirements(renderer->basics.device, stagingBuffer, &stagingRequirements);

	VkMemoryAllocateInfo stagingAllocateInfo{};
	stagingAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingAllocateInfo.allocationSize = stagingRequirements.size;
	stagingAllocateInfo.memoryTypeIndex = renderer->FindMemoryType(stagingRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkDeviceMemory stagingMemory;
	res = vkAllocateMemory(renderer->basics.device, &stagingAllocateInfo, nullptr, &stagingMemory);
	assert(res == VK_SUCCESS);

	vkBindBufferMemory(renderer->basics.device, stagingBuffer, stagingMemory, 0);

	uint8_t * const mappedMemory = [this](VkDeviceMemory const& memory, size_t size) {
		uint8_t * ret = nullptr;
		auto const res = vkMapMemory(renderer->basics.device, memory, 0, size, 0, (void **)&ret);
		assert(res == VK_SUCCESS);
		return ret;
	}(stagingMemory, stagingRequirements.size);

	for (size_t i = 0; i < size; ++i) {
		mappedMemory[i] = data[i];
	}

	vkUnmapMemory(renderer->basics.device, stagingMemory);

	auto cb = renderer->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cb, &beginInfo);
	renderer->CopyBufferToBuffer(cb, stagingBuffer, nativeHandle->buffer, offset, size);
	vkEndCommandBuffer(cb);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb;
	//TODO: Could submit and return a fence here that the user can use to know that their data has uploaded instead of using vkQueueWaitIdle
	vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(renderer->graphicsQueue);

	vkFreeCommandBuffers(renderer->basics.device, renderer->graphicsPool, 1, &cb);
	vkDestroyBuffer(renderer->basics.device, stagingBuffer, nullptr);
	vkFreeMemory(renderer->basics.device, stagingMemory, nullptr);
}

BufferHandle * VulkanResourceContext::CreateBuffer(BufferCreateInfo ci)
{
	auto const ret = (VulkanBufferHandle *)allocator.allocate(sizeof(VulkanBufferHandle));
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = ci.size;
	bufferInfo.usage = ci.usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	auto res = vkCreateBuffer(renderer->basics.device, &bufferInfo, nullptr, &ret->buffer);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(renderer->basics.device, ret->buffer, &memRequirements);

	VkMemoryAllocateInfo memInfo = {};
	memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memInfo.allocationSize = memRequirements.size;
	memInfo.memoryTypeIndex = renderer->FindMemoryType(memRequirements.memoryTypeBits, ci.memoryProperties);

	res = vkAllocateMemory(renderer->basics.device, &memInfo, nullptr, &ret->memory);
	assert(res == VK_SUCCESS);

	vkBindBufferMemory(renderer->basics.device, ret->buffer, ret->memory, 0);
	return ret;
}

void VulkanResourceContext::DestroyBuffer(BufferHandle * buffer)
{
	assert(buffer != nullptr);

	vkFreeMemory(renderer->basics.device, ((VulkanBufferHandle *)buffer)->memory, nullptr);
	allocator.destroy(buffer);
}

uint8_t * VulkanResourceContext::MapBuffer(BufferHandle * buffer, size_t offset, size_t size)
{
	assert(buffer != nullptr);

	uint8_t * ret;
	auto const res = vkMapMemory(renderer->basics.device, ((VulkanBufferHandle *)buffer)->memory, offset, size, 0, (void **)&ret);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::UnmapBuffer(BufferHandle * buffer)
{
	assert(buffer != nullptr);

	vkUnmapMemory(renderer->basics.device, ((VulkanBufferHandle *)buffer)->memory);
}

ImageHandle * VulkanResourceContext::CreateImage(ImageCreateInfo ci)
{
	VkImageType const imageType = [](ImageHandle::Type type) {
		switch (type) {
		case ImageHandle::Type::TYPE_1D:
			return VkImageType::VK_IMAGE_TYPE_1D;
		case ImageHandle::Type::TYPE_2D:
			return VkImageType::VK_IMAGE_TYPE_2D;
		default:
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
		(VkImageUsageFlags)ci.usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	auto const ret = (VulkanImageHandle *)allocator.allocate(sizeof(VulkanImageHandle));
	ret->depth = extent.depth;
	ret->format = ci.format;
	ret->height = extent.height;
	ret->type = ci.type;
	ret->width = extent.width;
	auto const res = vkCreateImage(renderer->basics.device, &info, nullptr, &ret->image);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyImage(ImageHandle * img)
{
	assert(img != nullptr);

	vkDestroyImage(renderer->basics.device, ((VulkanImageHandle *)img)->image, nullptr);
	allocator.destroy(img);
}

void VulkanResourceContext::ImageData(ImageHandle * img, std::vector<uint8_t> const& data)
{
	assert(img != nullptr);

	auto const nativeImg = (VulkanImageHandle *)img;

	VkMemoryRequirements const memoryRequirements = [this](VkImage img) {
		VkMemoryRequirements ret{0};
		vkGetImageMemoryRequirements(renderer->basics.device, img, &ret);
		return ret;
	}(nativeImg->image);

	assert(data.size() <= memoryRequirements.size);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(renderer->basics.physicalDevice, &memoryProperties);

	auto memoryIndex = renderer->FindMemoryType(memoryRequirements.memoryTypeBits, 0);

	VkMemoryAllocateInfo const info{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		memoryRequirements.size,
		memoryIndex
	};

	VkDeviceMemory const memory = [this](VkMemoryAllocateInfo const& info) {
		VkDeviceMemory ret{0};
		auto const res = vkAllocateMemory(renderer->basics.device, &info, nullptr, &ret);
		assert(res == VK_SUCCESS);
		return ret;
	}(info);

	VkBufferCreateInfo stagingInfo = {};
	stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingInfo.size = data.size();
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer stagingBuffer;
	auto res = vkCreateBuffer(renderer->basics.device, &stagingInfo, nullptr, &stagingBuffer);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements stagingRequirements = {};
	vkGetBufferMemoryRequirements(renderer->basics.device, stagingBuffer, &stagingRequirements);

	VkMemoryAllocateInfo stagingAllocateInfo{};
	stagingAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingAllocateInfo.allocationSize = stagingRequirements.size;
	stagingAllocateInfo.memoryTypeIndex = renderer->FindMemoryType(stagingRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkDeviceMemory stagingMemory;
	res = vkAllocateMemory(renderer->basics.device, &stagingAllocateInfo, nullptr, &stagingMemory);
	assert(res == VK_SUCCESS);

	vkBindBufferMemory(renderer->basics.device, stagingBuffer, stagingMemory, 0);

	uint8_t * const mappedMemory = [this](VkDeviceMemory const& memory, size_t size) {
		uint8_t * ret = nullptr;
		auto const res = vkMapMemory(renderer->basics.device, memory, 0, size, 0, (void **)&ret);
		assert(res == VK_SUCCESS);
		return ret;
	}(stagingMemory, stagingRequirements.size);

	for (size_t i = 0; i < data.size(); ++i) {
		mappedMemory[i] = data[i];
	}

	vkUnmapMemory(renderer->basics.device, stagingMemory);

	res = vkBindImageMemory(renderer->basics.device, nativeImg->image, memory, 0);
	
	auto cb = renderer->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cb, &beginInfo);
	renderer->TransitionImageLayout(cb, nativeImg->image, ToVulkanFormat(nativeImg->format), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	renderer->CopyBufferToImage(cb, stagingBuffer, nativeImg->image, nativeImg->width, nativeImg->height);
	renderer->TransitionImageLayout(cb, nativeImg->image, ToVulkanFormat(nativeImg->format), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vkEndCommandBuffer(cb);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb;
	vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(renderer->graphicsQueue);

	vkFreeCommandBuffers(renderer->basics.device, renderer->graphicsPool, 1, &cb);
	vkDestroyBuffer(renderer->basics.device, stagingBuffer, nullptr);
	vkFreeMemory(renderer->basics.device, stagingMemory, nullptr);

	assert(res == VK_SUCCESS);
}

RenderPassHandle * VulkanResourceContext::CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo ci)
{
	//TODO?
	std::vector<VkAttachmentDescription> attachments;
	if (ci.attachmentCount > 0) {
		attachments = std::vector<VkAttachmentDescription>(ci.attachmentCount);
		std::transform(ci.pAttachments, &ci.pAttachments[ci.attachmentCount], attachments.begin(), [](RenderPassHandle::AttachmentDescription description) {
			return VkAttachmentDescription{
				description.flags,
				ToVulkanFormat(description.format),
				VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
				ToVulkanLoadOp(description.loadOp),
				ToVulkanStoreOp(description.storeOp),
				ToVulkanLoadOp(description.stencilLoadOp),
				ToVulkanStoreOp(description.stencilStoreOp),
				ToVulkanImageLayout(description.initialLayout),
				ToVulkanImageLayout(description.finalLayout)
			};
		});
	}
	std::vector<VkAttachmentReference> colorAttachments;
	std::vector<VkAttachmentReference> depthStencilAttachments;
	std::vector<VkAttachmentReference> inputAttachments;

	std::vector<VkSubpassDescription> subpassDescriptions;
	if (ci.subpassCount > 0) {
		subpassDescriptions = std::vector<VkSubpassDescription>(ci.subpassCount);
		std::transform(ci.pSubpasses, &ci.pSubpasses[ci.subpassCount], subpassDescriptions.begin(), [&colorAttachments, &depthStencilAttachments, &inputAttachments](RenderPassHandle::SubpassDescription description) {
			auto firstColorAttachment = colorAttachments.size();
			for (uint32_t i = 0; i < description.colorAttachmentCount; ++i) {
				colorAttachments.push_back(ToVulkanAttachmentReference(description.pColorAttachments[i]));
			}
			auto firstDepthStencilAttachment = depthStencilAttachments.size();
			if (description.pDepthStencilAttachment != nullptr) {
				depthStencilAttachments.push_back(ToVulkanAttachmentReference(*description.pDepthStencilAttachment));
			}
			auto firstInputAttachment = inputAttachments.size();
			for (uint32_t i = 0; i < description.inputAttachmentCount; ++i) {
				inputAttachments.push_back(ToVulkanAttachmentReference(description.pInputAttachments[i]));
			}
			return VkSubpassDescription{
				0,
				ToVulkanPipelineBindPoint(description.pipelineBindPoint),
				description.inputAttachmentCount,
				inputAttachments.size() > 0 ? &inputAttachments[0] : nullptr,
				description.colorAttachmentCount,
				&colorAttachments[firstColorAttachment],
				//TODO
				nullptr,
				depthStencilAttachments.size() > 0 ? &depthStencilAttachments[firstDepthStencilAttachment] : nullptr,
				description.preserveAttachmentCount,
				description.pPreserveAttachments
			};
		});
	}

	std::vector<VkSubpassDependency> subpassDependencies;
	if (ci.dependencyCount > 0) {
		subpassDependencies = std::vector<VkSubpassDependency>(ci.dependencyCount);
		std::transform(ci.pDependencies, &ci.pDependencies[ci.dependencyCount], subpassDependencies.begin(), [](RenderPassHandle::SubpassDependency dependency) {
			return VkSubpassDependency{
				dependency.srcSubpass,
				dependency.dstSubpass,
				dependency.srcStageMask,
				dependency.dstStageMask,
				dependency.srcAccessMask,
				dependency.dstAccessMask,
				dependency.dependencyFlags
			};
		});
	}

	VkRenderPassCreateInfo const info{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		ci.attachmentCount,
		attachments.size() > 0 ? &attachments[0] : nullptr,
		ci.subpassCount,
		subpassDescriptions.size() > 0 ? &subpassDescriptions[0] : nullptr,
		ci.dependencyCount,
		subpassDependencies.size() > 0 ? &subpassDependencies[0] : nullptr
	};

	auto const ret = (VulkanRenderPassHandle *)allocator.allocate(sizeof(VulkanRenderPassHandle));
	auto const res = vkCreateRenderPass(renderer->basics.device, &info, nullptr, &ret->renderPass);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyRenderPass(RenderPassHandle * pass)
{
	assert(pass != nullptr);

	vkDestroyRenderPass(renderer->basics.device, ((VulkanRenderPassHandle *)pass)->renderPass, nullptr);
	allocator.destroy(pass);
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
		((VulkanImageHandle *)ci.image)->image,
		type,
		ToVulkanFormat(ci.format),
		ToVulkanComponentMapping(ci.components),
		ToVulkanSubResourceRange(ci.subresourceRange)
	};

	auto ret = (VulkanImageViewHandle *)allocator.allocate(sizeof(VulkanImageViewHandle));
	auto const res = vkCreateImageView(renderer->basics.device, &info, nullptr, &ret->imageView);
	assert(res == VK_SUCCESS);
	ret->image = ci.image;

	return ret;
}

void VulkanResourceContext::DestroyImageView(ImageViewHandle * view)
{
	assert(view != nullptr);

	vkDestroyImageView(renderer->basics.device, ((VulkanImageViewHandle *)view)->imageView, nullptr);
	allocator.destroy(view);
}

FramebufferHandle * VulkanResourceContext::CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const ci)
{
	std::vector<VkImageView> imageViews(ci.attachmentCount);
	std::transform(ci.pAttachments, &ci.pAttachments[ci.attachmentCount], imageViews.begin(), [](ImageViewHandle const * reference) {
		return ((VulkanImageViewHandle *)reference)->imageView;
	});
	VkFramebufferCreateInfo const info{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		nullptr,
		0,
		((VulkanRenderPassHandle *)ci.renderPass)->renderPass,
		ci.attachmentCount,
		&imageViews[0],
		ci.width,
		ci.height,
		ci.layers
	};

	auto ret = (VulkanFramebufferHandle *)allocator.allocate(sizeof(VulkanFramebufferHandle));
	auto const res = vkCreateFramebuffer(renderer->basics.device, &info, nullptr, &ret->framebuffer);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyFramebuffer(FramebufferHandle * fb)
{
	assert(fb != nullptr);

	vkDestroyFramebuffer(renderer->basics.device, ((VulkanFramebufferHandle *)fb)->framebuffer, nullptr);
	allocator.destroy(fb);
}

PipelineHandle * VulkanResourceContext::CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const ci)
{
	std::vector<uint32_t> specializationData{
		0
	};
	std::vector<VkSpecializationMapEntry> specializationMapEntries{
		{0, 0, sizeof(uint32_t)}
	};
	VkSpecializationInfo specializationInfo = {};
	specializationInfo.mapEntryCount = (uint32_t)specializationMapEntries.size();
	specializationInfo.pMapEntries = &specializationMapEntries[0];
	specializationInfo.dataSize = specializationData.size() * sizeof(uint32_t);
	specializationInfo.pData = &specializationData[0];

	std::vector<VkPipelineShaderStageCreateInfo> stages(ci.stageCount);
	
	std::transform(ci.pStages, &ci.pStages[ci.stageCount], stages.begin(), [&](ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stageInfo) {
		auto nativeShader = (VulkanShaderModuleHandle *)stageInfo.module;
		return VkPipelineShaderStageCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			ToVulkanShaderStage(stageInfo.stage),
			nativeShader->shader,
			"main",
			&specializationInfo
		};
	});

	auto nativeVertexInputState = ((VulkanVertexInputStateHandle *)ci.vertexInputState)->createInfo;
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(nativeVertexInputState.vertexBindingDescriptionCount);
	std::transform(nativeVertexInputState.pVertexBindingDescriptions, &nativeVertexInputState.pVertexBindingDescriptions[nativeVertexInputState.vertexBindingDescriptionCount], bindingDescriptions.begin(), [](VertexInputStateCreateInfo::VertexBindingDescription description) {
		return VkVertexInputBindingDescription{
			description.binding,
			description.stride,
			VK_VERTEX_INPUT_RATE_VERTEX
		};
	});

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(nativeVertexInputState.vertexAttributeDescriptionCount);
	std::transform(nativeVertexInputState.pVertexAttributeDescriptions, &nativeVertexInputState.pVertexAttributeDescriptions[nativeVertexInputState.vertexAttributeDescriptionCount], attributeDescriptions.begin(), [](VertexInputStateCreateInfo::VertexAttributeDescription description) {
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		if (description.type == VertexComponentType::FLOAT) {
			if (description.size == 1) {
				format = VK_FORMAT_R32_SFLOAT;
			} else if (description.size == 2) {
				format = VK_FORMAT_R32G32_SFLOAT;
			} else if (description.size == 3) {
				format = VK_FORMAT_R32G32B32_SFLOAT;
			} else if (description.size == 4) {
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}
		return VkVertexInputAttributeDescription{
			description.location,
			description.binding,
			format,
			description.offset
		};
	});

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		(uint32_t)bindingDescriptions.size(),
		&bindingDescriptions[0],
		(uint32_t)attributeDescriptions.size(),
		&attributeDescriptions[0]
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};

	VkRect2D scissor{
		{0, 0},
		renderer->swapchain.extent
	};
	VkViewport viewport{
		0.f,
		0.f,
		(float)renderer->swapchain.extent.width,
		(float)renderer->swapchain.extent.height,
		0.f,
		1.f
	};
	VkPipelineViewportStateCreateInfo viewportState{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&viewport,
		1,
		&scissor
	};

	VkPipelineRasterizationStateCreateInfo raserizationState{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,
		0.f,
		0.f,
		0.f,
		1.f
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		1.f,
		nullptr,
		VK_FALSE,
		VK_FALSE
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment{
		VK_FALSE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_LOGIC_OP_COPY,
		1,
		&colorBlendAttachment,
		{ 0.f, 0.f, 0.f, 0.f }
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&((VulkanDescriptorSetLayoutHandle *)ci.descriptorSetLayout)->layout,
		0,
		nullptr
	};

	VkPipelineLayout layout;
	{
		auto res = vkCreatePipelineLayout(renderer->basics.device, &layoutCreateInfo, nullptr, &layout);
		assert(res == VK_SUCCESS);
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = ci.stageCount;
	pipelineInfo.pStages = &stages[0];
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &raserizationState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = ((VulkanRenderPassHandle *)ci.renderPass)->renderPass;
	pipelineInfo.subpass = ci.subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto ret = (VulkanPipelineHandle *)allocator.allocate(sizeof(VulkanPipelineHandle));
	auto res = vkCreateGraphicsPipelines(renderer->basics.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ret->pipeline);
	assert(res == VK_SUCCESS);
	ret->descriptorSetLayout = ci.descriptorSetLayout;
	ret->vertexInputState = ci.vertexInputState;

	return ret;
}

void VulkanResourceContext::DestroyPipeline(PipelineHandle * pipeline)
{
	assert(pipeline != nullptr);

	vkDestroyPipeline(renderer->basics.device, ((VulkanPipelineHandle *)pipeline)->pipeline, nullptr);
	allocator.destroy(pipeline);
}

ShaderModuleHandle * VulkanResourceContext::CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo ci)
{
	VkShaderModuleCreateInfo info{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		ci.codeSize,
		(const uint32_t *)ci.pCode
	};

	auto ret = (VulkanShaderModuleHandle *)allocator.allocate(sizeof(ShaderModuleHandle *));
	auto const res = vkCreateShaderModule(renderer->basics.device, &info, nullptr, &ret->shader);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyShaderModule(ShaderModuleHandle * shader)
{
	assert(shader != nullptr);

	vkDestroyShaderModule(renderer->basics.device, ((VulkanShaderModuleHandle *)shader)->shader, nullptr);
	allocator.destroy(shader);
}

SamplerHandle * VulkanResourceContext::CreateSampler(ResourceCreationContext::SamplerCreateInfo ci)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = ToVulkanFilter(ci.magFilter);
	samplerInfo.minFilter = ToVulkanFilter(ci.minFilter);
	samplerInfo.addressModeU = ToVulkanAddressMode(ci.addressModeU);
	samplerInfo.addressModeV = ToVulkanAddressMode(ci.addressModeV);
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	auto ret = (VulkanSamplerHandle *)allocator.allocate(sizeof(VulkanSamplerHandle));
	auto res = vkCreateSampler(renderer->basics.device, &samplerInfo, nullptr, &ret->sampler);
	assert(res == VK_SUCCESS);

	return ret;
}

void VulkanResourceContext::DestroySampler(SamplerHandle * sampler)
{
	assert(sampler != nullptr);
	vkDestroySampler(renderer->basics.device, ((VulkanSamplerHandle *)sampler)->sampler, nullptr);
	allocator.destroy(sampler);
}

SemaphoreHandle * VulkanResourceContext::CreateSemaphore()
{
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto ret = (VulkanSemaphoreHandle *)allocator.allocate(sizeof(VulkanSemaphoreHandle));
	auto res = vkCreateSemaphore(renderer->basics.device, &createInfo, nullptr, &ret->semaphore);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroySemaphore(SemaphoreHandle * sem)
{
	assert(sem != nullptr);
	vkDestroySemaphore(renderer->basics.device, ((VulkanSemaphoreHandle *)sem)->semaphore, nullptr);
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
	auto res = vkCreateDescriptorSetLayout(renderer->basics.device, &ci, nullptr, &(ret->layout));
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle * layout)
{
	assert(layout != nullptr);
	vkDestroyDescriptorSetLayout(renderer->basics.device, ((VulkanDescriptorSetLayoutHandle *)layout)->layout, nullptr);
	allocator.destroy(layout);
}

FenceHandle * VulkanResourceContext::CreateFence(bool startSignaled)
{
	VkFenceCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	ci.flags = startSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
	
	auto mem = allocator.allocate(sizeof(VulkanFenceHandle));
	auto ret = new (mem) VulkanFenceHandle();
	ret->device = renderer->basics.device;
	auto res = vkCreateFence(renderer->basics.device, &ci, nullptr, &ret->fence);
	assert(res == VK_SUCCESS);
	return ret;
}

void VulkanResourceContext::DestroyFence(FenceHandle * fence)
{
	assert(fence != nullptr);
	auto nativeHandle = (VulkanFenceHandle *)fence;
	vkDestroyFence(renderer->basics.device, nativeHandle->fence, nullptr);
}

VertexInputStateHandle * VulkanResourceContext::CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo info)
{
	auto ret = (VulkanVertexInputStateHandle *)allocator.allocate(sizeof(VulkanVertexInputStateHandle));
	ret->createInfo = info;
	return ret;
}

void VulkanResourceContext::DestroyVertexInputState(VertexInputStateHandle * state)
{
	assert(state != nullptr);
	allocator.destroy(state);
}

DescriptorSet * VulkanResourceContext::CreateDescriptorSet(DescriptorSetCreateInfo info)
{
	assert(info.layout != nullptr);
	VkDescriptorSetLayout layout = ((VulkanDescriptorSetLayoutHandle *)info.layout)->layout;

	VkDescriptorSetAllocateInfo ai{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		renderer->descriptorPool,
		1,
		&layout
	};

	VkDescriptorSet set;
	auto res = vkAllocateDescriptorSets(renderer->basics.device, &ai, &set);
	assert(res == VK_SUCCESS);

	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
	std::vector<VkWriteDescriptorSet> writes(info.descriptorCount);
	std::transform(info.descriptors, &info.descriptors[info.descriptorCount], writes.begin(), [&](DescriptorSetCreateInfo::Descriptor descriptor) {
		size_t bufferInfoIdx = bufferInfos.size();
		size_t imageInfoIdx = imageInfos.size();

		if (descriptor.type == DescriptorType::UNIFORM_BUFFER) {
			auto buffer = std::get<ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor>(descriptor.descriptor);
			auto nativeBuffer = (VulkanBufferHandle *)buffer.buffer;
			bufferInfos.push_back({
				nativeBuffer->buffer,
				buffer.offset,
				buffer.range
			});
		} else if (descriptor.type == DescriptorType::COMBINED_IMAGE_SAMPLER) {
			auto image = std::get<ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor>(descriptor.descriptor);
			auto nativeImage = (VulkanImageHandle *)image.img;
			//TODO
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.maxAnisotropy = 1;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			VkSampler sampler;
			auto res = vkCreateSampler(renderer->basics.device, &samplerInfo, nullptr, &sampler);
			assert(res == VK_SUCCESS);

			VkImageViewCreateInfo imageViewInfo = {};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewInfo.format = ToVulkanFormat(nativeImage->format);
			imageViewInfo.image = nativeImage->image;
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.layerCount = 1;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			VkImageView imageView;
			res = vkCreateImageView(renderer->basics.device, &imageViewInfo, nullptr, &imageView);
			assert(res == VK_SUCCESS);

			imageInfos.push_back({
				sampler,
				imageView,
				VK_IMAGE_LAYOUT_GENERAL
			});
		}
		return VkWriteDescriptorSet{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			set,
			descriptor.binding,
			0,
			1,
			ToVulkanDescriptorType(descriptor.type),
			descriptor.type == DescriptorType::COMBINED_IMAGE_SAMPLER ? &imageInfos[imageInfoIdx] : nullptr,
			descriptor.type == DescriptorType::UNIFORM_BUFFER ? &bufferInfos[bufferInfoIdx] : nullptr,
			//TODO: if UNIFORM_TEXEL_BUFFER/STORAGE_TEXEL_BUFFER uniforms become available
			nullptr
		};
	});

	vkUpdateDescriptorSets(renderer->basics.device, (uint32_t)writes.size(), &writes[0], 0, nullptr);

	VkPipelineLayoutCreateInfo layoutCreateInfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&layout,
		0,
		nullptr
	};

	VkPipelineLayout pipeline_layout;
	{
		auto res = vkCreatePipelineLayout(renderer->basics.device, &layoutCreateInfo, nullptr, &pipeline_layout);
		assert(res == VK_SUCCESS);
	}

	auto ret = (VulkanDescriptorSet *)allocator.allocate(sizeof(VulkanDescriptorSet));
	ret->layout = layout;
	ret->pipelineLayout = pipeline_layout;
	ret->set = set;

	return ret;
}

void VulkanResourceContext::DestroyDescriptorSet(DescriptorSet * set)
{
	assert(set != nullptr);

	auto nativeDescriptorSet = (VulkanDescriptorSet *)set;

	auto res = vkFreeDescriptorSets(renderer->basics.device, renderer->descriptorPool, 1, &(nativeDescriptorSet->set));
	assert(res == VK_SUCCESS);
	vkDestroyDescriptorSetLayout(renderer->basics.device, nativeDescriptorSet->layout, nullptr);
	allocator.destroy(set);
}

RenderCommandContext * VulkanCommandContextAllocator::CreateContext(RenderCommandContextCreateInfo * pCreateInfo)
{
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = pCreateInfo->level == RenderCommandContextLevel::PRIMARY ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	VkCommandBuffer ret;
	vkAllocateCommandBuffers(device, &allocateInfo, &ret);
	auto retx = new VulkanRenderCommandContext(ret, new std::allocator<uint8_t>());
	return retx;
}

void VulkanCommandContextAllocator::DestroyContext(RenderCommandContext * ctx)
{
	assert(ctx != nullptr);
	auto nativeHandle = (VulkanRenderCommandContext *)ctx;
	vkFreeCommandBuffers(device, commandPool, 1, &nativeHandle->buffer);
}

void VulkanCommandContextAllocator::Reset()
{
	vkResetCommandPool(device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

bool VulkanFenceHandle::Wait(uint64_t timeOut)
{
	auto res = vkWaitForFences(device, 1, &fence, VK_TRUE, timeOut);
	vkResetFences(device, 1, &fence);
	return res != VK_TIMEOUT;
}

#endif
