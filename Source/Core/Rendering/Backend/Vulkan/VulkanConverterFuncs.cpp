#ifndef USE_OGL_RENDERER
#include "VulkanConverterFuncs.h"

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("VulkanConverterFuncs");

VkSamplerAddressMode ToVulkanAddressMode(AddressMode mode)
{
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

VkAttachmentReference ToVulkanAttachmentReference(RenderPassHandle::AttachmentReference const reference)
{
    return VkAttachmentReference{reference.attachment, ToVulkanImageLayout(reference.layout)};
}

uint32_t ToVulkanBufferUsageFlagBits(BufferUsageFlags flags)
{
    return (uint32_t)flags;
}

VkComponentMapping ToVulkanComponentMapping(ImageViewHandle::ComponentMapping mapping)
{
    return VkComponentMapping{
        ToVulkanComponentSwizzle(mapping.r),
        ToVulkanComponentSwizzle(mapping.g),
        ToVulkanComponentSwizzle(mapping.b),
        ToVulkanComponentSwizzle(mapping.a),
    };
}

VkComponentSwizzle ToVulkanComponentSwizzle(ComponentSwizzle swizzle)
{
    switch (swizzle) {
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

VkCullModeFlagBits ToVulkanCullMode(CullMode cullMode)
{
    switch (cullMode) {
    case CullMode::NONE:
        return VK_CULL_MODE_NONE;
    case CullMode::BACK:
        return VK_CULL_MODE_BACK_BIT;
    case CullMode::FRONT:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::FRONT_AND_BACK:
        return VK_CULL_MODE_FRONT_AND_BACK;
    default:
        assert(false);
        return VK_CULL_MODE_NONE;
    }
}

VkDescriptorType ToVulkanDescriptorType(DescriptorType descriptorType)
{
    switch (descriptorType) {
    case DescriptorType::COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    return VK_DESCRIPTOR_TYPE_SAMPLER;
}

VkFilter ToVulkanFilter(Filter filter)
{
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

VkFormat ToVulkanFormat(Format const & format)
{
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

VkFrontFace ToVulkanFrontFace(FrontFace frontFace)
{
    switch (frontFace) {
    case FrontFace::COUNTER_CLOCKWISE:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case FrontFace::CLOCKWISE:
        return VK_FRONT_FACE_CLOCKWISE;
    default:
        assert(false);
        return VK_FRONT_FACE_CLOCKWISE;
    }
}

VkImageLayout ToVulkanImageLayout(ImageLayout layout)
{
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
    logger->Errorf("Unknown image layout %d", layout);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAttachmentLoadOp ToVulkanLoadOp(RenderPassHandle::AttachmentDescription::LoadOp op)
{
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

VkPipelineBindPoint ToVulkanPipelineBindPoint(RenderPassHandle::PipelineBindPoint point)
{
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

VkShaderStageFlagBits ToVulkanShaderStage(uint32_t stage)
{
    return (VkShaderStageFlagBits)stage;
}

VkAttachmentStoreOp ToVulkanStoreOp(RenderPassHandle::AttachmentDescription::StoreOp op)
{
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

VkImageSubresourceRange ToVulkanSubResourceRange(ImageViewHandle::ImageSubresourceRange range)
{
    return VkImageSubresourceRange{
        range.aspectMask, range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount};
}

VkRect2D ToVulkanRect2D(CommandBuffer::Rect2D const & rect)
{
    return VkRect2D{{rect.offset.x, rect.offset.y}, {rect.extent.width, rect.extent.height}};
}

#endif
