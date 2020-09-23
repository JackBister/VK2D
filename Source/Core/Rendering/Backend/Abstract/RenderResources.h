#pragma once

#include <cstdint>
#include <optional>
#include <vector>

struct ImageViewHandle;
struct VertexInputStateHandle;

enum class AddressMode {
    REPEAT = 0,
    MIRRORED_REPEAT = 1,
    CLAMP_TO_EDGE = 2,
    CLAMP_TO_BORDER = 3,
    MIRROR_CLAMP_TO_EDGE = 4,
};

enum BufferUsageFlags {
    TRANSFER_SRC_BIT = 0x00000001,
    TRANSFER_DST_BIT = 0x00000002,
    UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
    STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
    UNIFORM_BUFFER_BIT = 0x00000010,
    STORAGE_BUFFER_BIT = 0x00000020,
    INDEX_BUFFER_BIT = 0x00000040,
    VERTEX_BUFFER_BIT = 0x00000080,
    INDIRECT_BUFFER_BIT = 0x00000100,
};

enum CommandBufferUsageFlagBits {
    ONE_TIME_SUBMIT_BIT = 0x00000001,
    RENDER_PASS_CONTINUE_BIT = 0x00000002,
    SIMULTANEOUS_USE_BIT = 0x00000004,
};

enum class CompareOp {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
};

enum class ComponentSwizzle { IDENTITY, ZERO, ONE, R, G, B, A };

enum class CullMode { NONE, BACK, FRONT, FRONT_AND_BACK };

enum class DependencyFlagBits { BY_REGION_BIT = 0x00000001 };

enum class DescriptorType {
    /*
    SAMPLER = 0,
    */
    COMBINED_IMAGE_SAMPLER = 1,
    /*
    SAMPLED_IMAGE = 2,
    STORAGE_IMAGE = 3,
    UNIFORM_TEXEL_BUFFER = 4,
    STORAGE_TEXEL_BUFFER = 5,
    */
    UNIFORM_BUFFER = 6,
    /*
    STORAGE_BUFFER = 7,
    UNIFORM_BUFFER_DYNAMIC = 8,
    STORAGE_BUFFER_DYNAMIC = 9,
    INPUT_ATTACHMENT = 10,
    */
};

enum class Format {
    B8G8R8A8_UNORM,

    R8,
    RG8,
    RGB8,
    RGBA8,

    R16G16_SFLOAT,

    R32G32B32A32_SFLOAT,

    D32_SFLOAT,
};

enum class FrontFace {
    COUNTER_CLOCKWISE,
    CLOCKWISE,
};

enum class ImageLayout {
    UNDEFINED,
    GENERAL,
    COLOR_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    SHADER_READ_ONLY_OPTIMAL,
    TRANSFER_SRC_OPTIMAL,
    TRANSFER_DST_OPTIMAL,
    PREINITIALIZED,
    PRESENT_SRC_KHR
};

enum ImageUsageFlagBits {
    IMAGE_USAGE_FLAG_TRANSFER_SRC_BIT = 0x00000001,
    IMAGE_USAGE_FLAG_TRANSFER_DST_BIT = 0x00000002,
    IMAGE_USAGE_FLAG_SAMPLED_BIT = 0x00000004,
    IMAGE_USAGE_FLAG_STORAGE_BIT = 0x00000008,
    IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT = 0x00000010,
    IMAGE_USAGE_FLAG_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    IMAGE_USAGE_FLAG_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
    IMAGE_USAGE_FLAG_INPUT_ATTACHMENT_BIT = 0x00000080,
};

enum class Filter { NEAREST = 0, LINEAR = 1 };

enum MemoryPropertyFlagBits {
    DEVICE_LOCAL_BIT = 0x00000001,
    HOST_VISIBLE_BIT = 0x00000002,
    HOST_COHERENT_BIT = 0x00000004,
    HOST_CACHED_BIT = 0x00000008,
    LAZILY_ALLOCATED_BIT = 0x00000010,
};

enum PipelineStageFlagBits {
    TOP_OF_PIPE_BIT = 0x00000001,
    DRAW_INDIRECT_BIT = 0x00000002,
    VERTEX_INPUT_BIT = 0x00000004,
    VERTEX_SHADER_BIT = 0x00000008,
    TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
    TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
    GEOMETRY_SHADER_BIT = 0x00000040,
    FRAGMENT_SHADER_BIT = 0x00000080,
    EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
    LATE_FRAGMENT_TESTS_BIT = 0x00000200,
    COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
    COMPUTE_SHADER_BIT = 0x00000800,
    TRANSFER_BIT = 0x00001000,
    BOTTOM_OF_PIPE_BIT = 0x00002000,
    HOST_BIT = 0x00004000,
    ALL_GRAPHICS_BIT = 0x00008000,
    ALL_COMMANDS_BIT = 0x00010000,
};

enum class PrimitiveTopology {
    POINT_LIST = 0,
    LINE_LIST = 1,
    TRIANGLE_LIST = 3,
};

enum class CommandBufferLevel { PRIMARY, SECONDARY };

enum ShaderStageFlagBits {
    SHADER_STAGE_VERTEX_BIT = 0x00000001,
    SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
    SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
    SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
    SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
    SHADER_STAGE_COMPUTE_BIT = 0x00000020,
    SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
    SHADER_STAGE_ALL = 0x7FFFFFFF,
};

// TODO: Will be changed in the future when API converges with Vulkan more
enum class VertexComponentType { BYTE, SHORT, INT, FLOAT, HALF_FLOAT, DOUBLE, UBYTE, USHORT, UINT };

struct BufferHandle {
};

struct DescriptorSet {
};

struct DescriptorSetLayoutHandle {
};

struct FenceHandle {
    virtual bool Wait(uint64_t timeOut) = 0;
};

struct FramebufferHandle {
};

struct ImageHandle {
    enum class Type { TYPE_1D, TYPE_2D };
    Format format;
    Type type;
    uint32_t width, height, depth;
};

struct ImageViewHandle {
    enum ImageAspectFlagBits { COLOR_BIT = 0x00000001, DEPTH_BIT = 0x00000002, STENCIL_BIT = 0x00000004 };
    enum class Type { TYPE_1D, TYPE_2D, TYPE_3D, TYPE_CUBE, TYPE_1D_ARRAY, TYPE_2D_ARRAY, CUBE_ARRAY };
    struct ComponentMapping {
        ComponentSwizzle r;
        ComponentSwizzle g;
        ComponentSwizzle b;
        ComponentSwizzle a;

        static const ComponentMapping IDENTITY;
    };

    struct ImageSubresourceRange {
        uint32_t aspectMask;
        uint32_t baseMipLevel;
        uint32_t levelCount;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };
};

struct PipelineLayoutHandle {
};

struct PipelineHandle {
    VertexInputStateHandle * vertexInputState;
};

struct RenderPassHandle {
    enum class PipelineBindPoint { GRAPHICS, COMPUTE };
    struct AttachmentDescription {
        enum class Flags : uint8_t {
            MAY_ALIAS_BIT = 0x01,
        };
        enum class LoadOp { LOAD, CLEAR, DONT_CARE };
        enum class StoreOp { STORE, DONT_CARE };
        uint8_t flags;
        Format format;
        LoadOp loadOp;
        StoreOp storeOp;
        LoadOp stencilLoadOp;
        StoreOp stencilStoreOp;
        ImageLayout initialLayout;
        ImageLayout finalLayout;
    };

    struct AttachmentReference {
        uint32_t attachment;
        ImageLayout layout;
    };

    struct SubpassDescription {
        PipelineBindPoint pipelineBindPoint;
        std::vector<AttachmentReference> inputAttachments;
        std::vector<AttachmentReference> colorAttachments;
        std::vector<AttachmentReference> resolveAttachments;
        std::optional<AttachmentReference> depthStencilAttachment;
        std::vector<uint32_t> preserveAttachments;
    };

    struct SubpassDependency {
        uint32_t srcSubpass;
        uint32_t dstSubpass;
        uint32_t srcStageMask;
        uint32_t dstStageMask;
        uint32_t srcAccessMask;
        uint32_t dstAccessMask;
        uint32_t dependencyFlags;
    };
};

struct SamplerHandle {
};

struct SemaphoreHandle {
};

struct ShaderModuleHandle {
};

struct VertexInputStateHandle {
};
