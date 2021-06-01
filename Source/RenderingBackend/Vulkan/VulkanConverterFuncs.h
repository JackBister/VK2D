#pragma once
#ifndef USE_OGL_RENDERER

#include <cassert>
#include <vulkan/vulkan.h>

#include "../Abstract/CommandBuffer.h"
#include "../Abstract/RenderResources.h"

VkSamplerAddressMode ToVulkanAddressMode(AddressMode mode);
VkAttachmentReference ToVulkanAttachmentReference(RenderPassHandle::AttachmentReference const reference);
uint32_t ToVulkanBufferUsageFlagBits(BufferUsageFlags flags);
VkCompareOp ToVulkanCompareOp(CompareOp compareOp);
VkComponentMapping ToVulkanComponentMapping(ImageViewHandle::ComponentMapping mapping);
VkComponentSwizzle ToVulkanComponentSwizzle(ComponentSwizzle swizzle);
VkCullModeFlagBits ToVulkanCullMode(CullMode cullMode);
bool IsImageDescriptorType(DescriptorType descriptorType);
VkDescriptorType ToVulkanDescriptorType(DescriptorType descriptorType);
VkFilter ToVulkanFilter(Filter filter);
VkFormat ToVulkanFormat(Format const & format);
VkFrontFace ToVulkanFrontFace(FrontFace frontFace);
VkImageLayout ToVulkanImageLayout(ImageLayout layout);
VkAttachmentLoadOp ToVulkanLoadOp(RenderPassHandle::AttachmentDescription::LoadOp op);
VkPipelineBindPoint ToVulkanPipelineBindPoint(RenderPassHandle::PipelineBindPoint point);
VkShaderStageFlagBits ToVulkanShaderStage(uint32_t stage);
VkAttachmentStoreOp ToVulkanStoreOp(RenderPassHandle::AttachmentDescription::StoreOp op);
VkImageSubresourceRange ToVulkanSubResourceRange(ImageViewHandle::ImageSubresourceRange range);
VkRect2D ToVulkanRect2D(CommandBuffer::Rect2D const & rect);

#endif
