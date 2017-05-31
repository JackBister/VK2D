#pragma once
#include "Core/Rendering/Context/RenderContext.h"

#include <vector>

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif
#include <gl/glew.h>

struct OpenGLRenderCommandContext : RenderCommandContext
{
	friend struct Renderer;

	OpenGLRenderCommandContext() = delete;
	// Inherited via RenderCommandContext
	virtual void CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo *pRenderPassBegin, RenderCommandContext::SubpassContents contents) override;
	virtual void CmdBindDescriptorSet(DescriptorSet *) override;
	virtual void CmdBindIndexBuffer(BufferHandle *buffer, size_t offset, RenderCommandContext::IndexType indexType) override;
	virtual void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) override;
	virtual void CmdBindUniformBuffer(uint32_t binding, BufferHandle *, uint32_t offset, uint32_t size) override;
	virtual void CmdBindUniformImage(uint32_t binding, ImageHandle *) override;
	virtual void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) override;
	virtual void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) override;
	virtual void CmdEndRenderPass() override;
	virtual void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const RenderCommandContext::Rect2D *pScissors) override;
	virtual void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const RenderCommandContext::Viewport *pViewports) override;
	virtual void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, const uint32_t * pData) override;

	// Inherited via RenderCommandContext

protected:
	// Inherited via RenderCommandContext
	virtual void Execute() override;
private:
	OpenGLRenderCommandContext(std::allocator<uint8_t> * allocator) : RenderCommandContext(allocator) {}
	/*
		Render command types and args:
	*/
	struct BeginRenderPassArgs
	{
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
	struct BindUniformBufferArgs
	{
		GLuint index;
		GLuint buffer;
		GLintptr offset;
		GLsizeiptr size;
	};
	struct BindUniformImageArgs
	{
		uint32_t binding;
		GLuint image;
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
		const void * indices;
		GLsizei primcount;
	};
	struct EndRenderPassArgs
	{
	};
	struct SetScissorArgs
	{
		GLuint first;
		GLsizei count;
		const GLint * v;
	};
	struct SetViewportArgs
	{
		GLuint first;
		GLsizei count;
		const GLfloat * v;
	};
	struct UpdateBufferArgs
	{
		GLuint buffer;
		GLintptr offset;
		GLsizeiptr size;
		const void * data;
	};
	using RenderCommand = std::variant<BeginRenderPassArgs, BindDescriptorSetArgs, BindIndexBufferArgs, BindPipelineArgs,
									   BindUniformBufferArgs, BindUniformImageArgs, BindVertexBufferArgs,
									   DrawIndexedArgs, EndRenderPassArgs, SetScissorArgs, SetViewportArgs,
									   UpdateBufferArgs> ;

	/*
		The list of commands to execute
	*/
	std::vector<RenderCommand> commandList;

	/*
		State trackers
	*/
	size_t indexBufferOffset;
	GLenum indexBufferType;
};

struct OpenGLResourceContext : ResourceCreationContext
{
	BufferHandle * CreateBuffer(BufferCreateInfo) final override;
	void DestroyBuffer(BufferHandle *) final override;
	uint8_t * MapBuffer(BufferHandle *, size_t, size_t) final override;
	void UnmapBuffer(BufferHandle *) final override;
	ImageHandle * CreateImage(ImageCreateInfo) final override;
	void DestroyImage(ImageHandle *) final override;
	void ImageData(ImageHandle *, const std::vector<uint8_t>&) final override;

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
	virtual void DestroyVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo *);

	virtual DescriptorSet * CreateDescriptorSet(DescriptorSetCreateInfo) override;
	virtual void DestroyDescriptorSet(DescriptorSet *) override;
};

struct OpenGLBufferHandle : BufferHandle
{
	GLuint nativeHandle = 0;
	size_t offset = 0;
	GLenum elementType = 0;
};

struct OpenGLDescriptorSet : DescriptorSet
{
	std::vector<ResourceCreationContext::DescriptorSetCreateInfo::Descriptor> descriptors;
};

struct OpenGLDescriptorSetLayoutHandle : DescriptorSetLayoutHandle
{
	std::vector<ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding> bindings;
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

struct OpenGLShaderModuleHandle : ShaderModuleHandle
{
	GLuint nativeHandle;
};

struct OpenGLVertexInputStateHandle : VertexInputStateHandle
{
	//TODO: Is a VAO sufficient?
	GLuint nativeHandle;
};
