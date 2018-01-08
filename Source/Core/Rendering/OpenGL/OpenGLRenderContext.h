#pragma once
#ifndef USE_VULKAN_RENDERER
#include "Core/Rendering/Context/RenderContext.h"

#include <vector>

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif
#include <gl/glew.h>
#include <SDL/SDL.h>

#include "Core/Semaphore.h"

class OpenGLRenderCommandContext : public RenderCommandContext
{
	friend class Renderer;
public:

	OpenGLRenderCommandContext() : RenderCommandContext(new std::allocator<uint8_t>) {}
	OpenGLRenderCommandContext(std::allocator<uint8_t> * allocator) : RenderCommandContext(allocator) {}

	~OpenGLRenderCommandContext() final override
	{
		delete allocator;
	}

	 void BeginRecording(InheritanceInfo *) final override;
	 void EndRecording() final override;

	 void CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo * pRenderPassBegin, RenderCommandContext::SubpassContents contents) final override;
	 void CmdBindDescriptorSet(DescriptorSet *) final override;
	 void CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, RenderCommandContext::IndexType indexType) final override;
	 void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) final override;
	 void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) final override;
	 void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) final override;
	 void CmdEndRenderPass() final override;
	 void CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers) final override;
	 void CmdExecuteCommands(std::vector<RenderCommandContext *>&& commandBuffers) final override;
	 void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, RenderCommandContext::Rect2D const * pScissors) final override;
	 void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, RenderCommandContext::Viewport const * pViewports) final override;
	 void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) final override;

protected:
	void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem) override;
private:
	enum RenderCommandType
	{
		BEGIN_RENDERPASS,
		BIND_DESCRIPTOR_SET,
		BIND_INDEX_BUFFER,
		BIND_PIPELINE,
		BIND_VERTEX_BUFFER,
		DRAW_INDEXED,
		END_RENDERPASS,
		EXECUTE_COMMANDS,
		EXECUTE_COMMANDS_VECTOR,
		SET_SCISSOR,
		SET_VIEWPORT,
		SWAP_WINDOW,
		UPDATE_BUFFER
	};
	/*
		Render command types and args:
	*/
	struct BeginRenderPassArgs
	{
		RenderPassHandle * renderPass;
		FramebufferHandle * framebuffer;
		uint32_t clearValueCount;
		ClearValue * pClearValues;
	};
	struct BindDescriptorSetArgs
	{
		struct GLBufferDescriptor
		{
			GLuint binding;
			GLuint buffer;
			GLintptr offset;
			GLsizeiptr size;
		};
		struct GLImageDescriptor
		{
			GLuint binding;
			GLuint image;
		};
		std::vector<GLBufferDescriptor> buffers;
		std::vector<GLImageDescriptor> images;
	};
	struct BindIndexBufferArgs
	{
		GLuint buffer;
		size_t offset;
		GLenum indexType;
	};
	struct BindPipelineArgs
	{
		GLuint program;
		GLuint vao;
	};
	struct BindVertexBufferArgs
	{
		GLuint buffer;
		uint32_t binding;
		size_t offset;
		uint32_t stride;
	};
	struct DrawIndexedArgs
	{
		GLsizei count;
		//Why the hell does OpenGL want a pointer for this?
		void const * indices;
		GLsizei primcount;
	};
	struct EndRenderPassArgs
	{
	};
	struct ExecuteCommandsArgs
	{
		uint32_t commandBufferCount;
		OpenGLRenderCommandContext ** pCommandBuffers;
	};
	struct ExecuteCommandsVectorArgs
	{
		std::vector<RenderCommandContext *> commandBuffers;
	};
	struct SetScissorArgs
	{
		GLuint first;
		GLsizei count;
		GLint const * v;
	};
	struct SetViewportArgs
	{
		GLuint first;
		GLsizei count;
		GLfloat const * v;
	};
	struct SwapWindowArgs
	{
	};
	struct UpdateBufferArgs
	{
		GLuint buffer;
		GLintptr offset;
		GLsizeiptr size;
		void const * data;
	};
	using RenderCommand = std::variant<BeginRenderPassArgs, BindDescriptorSetArgs, BindIndexBufferArgs, BindPipelineArgs,
									   BindVertexBufferArgs, DrawIndexedArgs, EndRenderPassArgs, ExecuteCommandsArgs,
									   ExecuteCommandsVectorArgs, SetScissorArgs, SetViewportArgs, SwapWindowArgs, UpdateBufferArgs>;

	/*
		The list of commands to execute
	*/
	std::vector<RenderCommand> commandList;

	/*
		State trackers
	*/
	size_t indexBufferOffset;
	GLenum indexBufferType;

	// Inherited via RenderCommandContext
	virtual void Reset();
};

class OpenGLResourceContext : public ResourceCreationContext
{
public:
	virtual RenderCommandContext * CreateCommandContext(RenderCommandContextCreateInfo * pCreateInfo) final override;
	virtual void DestroyCommandContext(RenderCommandContext *) final override;

	void BufferSubData(BufferHandle *, uint8_t *, size_t, size_t) final override;
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
	virtual void DestroyVertexInputState(VertexInputStateHandle *);

	virtual DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo) override;
	virtual void DestroyDescriptorSet(DescriptorSet *) override;

	// Inherited via ResourceCreationContext
	virtual SemaphoreHandle * CreateSemaphore();
	virtual void DestroySemaphore(SemaphoreHandle *);

	// Inherited via ResourceCreationContext
	virtual CommandContextAllocator * CreateCommandContextAllocator();
	virtual void DestroyCommandContextAllocator(CommandContextAllocator *);
	virtual FenceHandle * CreateFence(bool startSignaled);
	virtual void DestroyFence(FenceHandle *);
};

struct OpenGLBufferHandle : BufferHandle
{
	GLuint nativeHandle = 0;
	size_t offset = 0;
	GLenum elementType = 0;
};

struct OpenGLCommandContextAllocator : CommandContextAllocator
{
	RenderCommandContext * CreateContext(RenderCommandContextCreateInfo * pCreateInfo) final override;

	void DestroyContext(RenderCommandContext *) final override;

	void Reset() final override;

};

struct OpenGLDescriptorSet : DescriptorSet
{
	std::vector<ResourceCreationContext::DescriptorSetCreateInfo::Descriptor> descriptors;
};

struct OpenGLDescriptorSetLayoutHandle : DescriptorSetLayoutHandle
{
	std::vector<ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding> bindings;
};

struct OpenGLFenceHandle : FenceHandle
{
	bool Wait(uint64_t timeOut) final override;

	Semaphore sem;
};

struct OpenGLFramebufferHandle : FramebufferHandle
{
	GLuint nativeHandle = 0;
};

struct OpenGLImageHandle : ImageHandle
{
	GLuint nativeHandle = 0;
};

struct OpenGLImageViewHandle : ImageViewHandle
{

};

struct OpenGLRenderPassHandle : RenderPassHandle
{

};

struct OpenGLPipelineHandle : PipelineHandle
{
	GLuint nativeHandle;
};

struct OpenGLSamplerHandle : SamplerHandle
{
	GLuint nativeHandle;
};

struct OpenGLSemaphoreHandle : SemaphoreHandle
{
	Semaphore sem;
};

struct OpenGLShaderModuleHandle : ShaderModuleHandle
{
	GLuint nativeHandle;
};

struct OpenGLVertexInputStateHandle : VertexInputStateHandle
{
	//TODO: Is a VAO sufficient?
	GLuint nativeHandle;
};
#endif
