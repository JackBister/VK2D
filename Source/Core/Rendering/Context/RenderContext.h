#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct ImageViewHandle;
class CommandBuffer;
struct VertexInputStateHandle;

enum class AccessFlagBits
{
	INDIRECT_COMMAND_READ_BIT = 0x00000001,
	INDEX_READ_BIT = 0x00000002,
	VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
	UNIFORM_READ_BIT = 0x00000008,
	INPUT_ATTACHMENT_READ_BIT = 0x00000010,
	SHADER_READ_BIT = 0x00000020,
	SHADER_WRITE_BIT = 0x00000040,
	COLOR_ATTACHMENT_READ_BIT = 0x00000080,
	COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
	DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
	DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
	TRANSFER_READ_BIT = 0x00000800,
	TRANSFER_WRITE_BIT = 0x00001000,
	HOST_READ_BIT = 0x00002000,
	HOST_WRITE_BIT = 0x00004000,
	MEMORY_READ_BIT = 0x00008000,
	MEMORY_WRITE_BIT = 0x00010000,
};

enum class AddressMode
{
	REPEAT = 0,
	MIRRORED_REPEAT = 1,
	CLAMP_TO_EDGE = 2,
	CLAMP_TO_BORDER = 3,
	MIRROR_CLAMP_TO_EDGE = 4,
};

enum BufferUsageFlags
{
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

enum class ComponentSwizzle
{
	IDENTITY,
	ZERO,
	ONE,
	R,
	G,
	B,
	A
};

enum class CullMode
{
	NONE,
	BACK,
	FRONT,
	FRONT_AND_BACK
};

enum class DependencyFlagBits
{
	BY_REGION_BIT = 0x00000001
};

enum class DescriptorType
{
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

enum class Format
{
	B8G8R8A8_UNORM,

	R8,
	RG8,
	RGB8,
	RGBA8
};

enum class FrontFace
{
	COUNTER_CLOCKWISE,
	CLOCKWISE,
};

enum class ImageLayout
{
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

enum class Filter
{
	NEAREST = 0,
	LINEAR = 1
};

enum MemoryPropertyFlagBits
{
	DEVICE_LOCAL_BIT = 0x00000001,
	HOST_VISIBLE_BIT = 0x00000002,
	HOST_COHERENT_BIT = 0x00000004,
	HOST_CACHED_BIT = 0x00000008,
	LAZILY_ALLOCATED_BIT = 0x00000010,
};

enum PipelineStageFlagBits
{
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

enum class CommandBufferLevel
{
	PRIMARY,
	SECONDARY
};

enum ShaderStageFlagBits
{
	SHADER_STAGE_VERTEX_BIT = 0x00000001,
	SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
	SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
	SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
	SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
	SHADER_STAGE_COMPUTE_BIT = 0x00000020,
	SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
	SHADER_STAGE_ALL = 0x7FFFFFFF,
};

//TODO: Will be changed in the future when API converges with Vulkan more
enum class VertexComponentType
{
	BYTE,
	SHORT,
	INT,
	FLOAT,
	HALF_FLOAT,
	DOUBLE,
	UBYTE,
	USHORT,
	UINT
};

struct BufferHandle
{
};

struct CommandBufferAllocator
{
	struct CommandBufferCreateInfo
	{
		CommandBufferLevel level;
	};
	virtual CommandBuffer * CreateBuffer(CommandBufferCreateInfo const& pCreateInfo) = 0;
	virtual void DestroyContext(CommandBuffer *) = 0;
	virtual void Reset() = 0;
};

struct DescriptorSet
{
};

struct DescriptorSetLayoutHandle
{
};

struct FenceHandle
{
	virtual bool Wait(uint64_t timeOut) = 0;
};

struct FramebufferHandle
{
	uint32_t attachmentCount;
	Format format;
	ImageViewHandle const ** pAttachments;
	uint32_t width, height, layers;
};

struct ImageHandle
{
	enum class Type
	{
		TYPE_1D,
		TYPE_2D
	};
	Format format;
	Type type;
	uint32_t width, height, depth;

	uint32_t DEBUGUINT = 0;
};

struct ImageViewHandle
{
	enum ImageAspectFlagBits
	{
		COLOR_BIT = 0x00000001,
		DEPTH_BIT = 0x00000002,
		STENCIL_BIT = 0x00000004
	};
	enum class Type
	{
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
		TYPE_CUBE,
		TYPE_1D_ARRAY,
		TYPE_2D_ARRAY,
		CUBE_ARRAY
	};
	struct ComponentMapping
	{
		ComponentSwizzle r;
		ComponentSwizzle g;
		ComponentSwizzle b;
		ComponentSwizzle a;

		static const ComponentMapping IDENTITY;
	};

	struct ImageSubresourceRange
	{
		uint32_t aspectMask;
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	ImageHandle * image;
	ImageSubresourceRange subresourceRange;
};

struct PipelineHandle
{
	VertexInputStateHandle * vertexInputState;
	DescriptorSetLayoutHandle * descriptorSetLayout;
};

struct RenderPassHandle
{
	enum class PipelineBindPoint
	{
		GRAPHICS,
		COMPUTE
	};
	struct AttachmentDescription
	{
		enum class Flags : uint8_t
		{
			MAY_ALIAS_BIT = 0x01,
		};
		enum class LoadOp
		{
			LOAD,
			CLEAR,
			DONT_CARE
		};
		enum class StoreOp
		{
			STORE,
			DONT_CARE
		};
		uint8_t flags;
		Format format;
		LoadOp loadOp;
		StoreOp storeOp;
		LoadOp stencilLoadOp;
		StoreOp stencilStoreOp;
		ImageLayout initialLayout;
		ImageLayout finalLayout;
	};

	struct AttachmentReference
	{
		uint32_t attachment;
		ImageLayout layout;
	};

	struct SubpassDescription
	{
		PipelineBindPoint pipelineBindPoint;
		uint32_t inputAttachmentCount;
		AttachmentReference const * pInputAttachments;
		uint32_t colorAttachmentCount;
		AttachmentReference const * pColorAttachments;
		AttachmentReference const * pResolveAttachments;
		AttachmentReference const * pDepthStencilAttachment;
		uint32_t preserveAttachmentCount;
		uint32_t const * pPreserveAttachments;
	};
	
	struct SubpassDependency
	{
		uint32_t srcSubpass;
		uint32_t dstSubpass;
		uint32_t srcStageMask;
		uint32_t dstStageMask;
		uint32_t srcAccessMask;
		uint32_t dstAccessMask;
		uint32_t dependencyFlags;
	};

};

struct SamplerHandle
{
};

struct SemaphoreHandle
{
};

struct ShaderModuleHandle
{
};

struct VertexInputStateHandle
{
};

class CommandBuffer
{
	friend class Renderer;
public:

	enum class IndexType
	{
		UINT16,
		UINT32
	};
	enum class SubpassContents
	{
		INLINE,
		SECONDARY_COMMAND_BUFFERS
	};
	struct ClearValue
	{
		enum class Type
		{
			COLOR,
			DEPTH_STENCIL
		};
		Type type;
		union
		{
			union
			{
				float float32[4];
				int32_t int32[4];
				uint32_t uint32[4];
			} color;
			struct
			{
				float depth;
				uint32_t stencil;
			} depthStencil;
		};
	};
	struct Rect2D
	{
		struct
		{
			int32_t x;
			int32_t y;
		} offset;
		struct
		{
			uint32_t width;
			uint32_t height;
		} extent;
	};
	struct RenderPassBeginInfo
	{
		RenderPassHandle * renderPass;
		FramebufferHandle * framebuffer;
		Rect2D renderArea;
		uint32_t clearValueCount;
		ClearValue const * pClearValues;
	};
	struct Viewport
	{
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	CommandBuffer(std::allocator<uint8_t> * allocator) : allocator(allocator) {}
	virtual ~CommandBuffer() {}

	struct InheritanceInfo
	{
		RenderPassHandle * renderPass;
		uint32_t subpass;
		FramebufferHandle * framebuffer;
		uint32_t commandBufferUsageFlags;
	};
	virtual void BeginRecording(InheritanceInfo *) = 0;
	virtual void EndRecording() = 0;
	virtual void Reset() = 0;

	virtual void CmdBeginRenderPass(RenderPassBeginInfo * pRenderPassBegin, SubpassContents contents) = 0;
	virtual void CmdBindDescriptorSet(DescriptorSet *) = 0;
	/*
		OpenGL: glBindBuffer, state tracker saves offset and index type until glDrawElements is issued
	*/
	virtual void CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, IndexType indexType) = 0;
	virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) = 0;
	//TODO: Stride not necessary in Vulkan - Can hack around it in OpenGL too
	virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) = 0;
	virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) = 0;
	virtual void CmdEndRenderPass() = 0;
	virtual void CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers) = 0;
	//TODO:
	virtual void CmdExecuteCommands(std::vector<CommandBuffer *>&& commandBuffers) = 0;
	virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, Rect2D const * pScissors) = 0;
	virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, Viewport const * pViewports) = 0;

	virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) = 0;

protected:
	virtual void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence) = 0;

	//TODO: Stack allocator for throwaway arrays
	std::allocator<uint8_t> * allocator;
};

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
