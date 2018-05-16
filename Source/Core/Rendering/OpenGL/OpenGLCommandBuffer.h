#pragma once
#ifdef USE_OGL_RENDERER
#include "Core/Rendering/Context/RenderContext.h"

#include <variant>
#include <vector>

#include <gl/glew.h>
#include <SDL2/SDL.h>

#include "Core/Semaphore.h"

class OpenGLCommandBuffer : public CommandBuffer
{
	friend class Renderer;
public:

	OpenGLCommandBuffer(std::allocator<uint8_t> * allocator = new std::allocator<uint8_t>) : CommandBuffer(allocator) {}

	~OpenGLCommandBuffer() final override
	{
		delete allocator;
	}

	 void BeginRecording(InheritanceInfo *) final override;
	 void EndRecording() final override;

	 void CmdBeginRenderPass(CommandBuffer::RenderPassBeginInfo * pRenderPassBegin, CommandBuffer::SubpassContents contents) final override;
	 void CmdBindDescriptorSet(DescriptorSet *) final override;
	 void CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, CommandBuffer::IndexType indexType) final override;
	 void CmdBindPipeline(RenderPassHandle::PipelineBindPoint, PipelineHandle *) final override;
	 void CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride) final override;
	 void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset) final override;
	 void CmdEndRenderPass() final override;
	 void CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers) final override;
	 void CmdExecuteCommands(std::vector<CommandBuffer *>&& commandBuffers) final override;
	 void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, CommandBuffer::Rect2D const * pScissors) final override;
	 void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, CommandBuffer::Viewport const * pViewports) final override;
	 void CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData) final override;

protected:
	void Execute(Renderer *, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence) override;
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
			GLuint sampler;
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
		GLenum cullMode;
		GLenum frontFace;
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
		GLint basevertex;
	};
	struct EndRenderPassArgs
	{
	};
	struct ExecuteCommandsArgs
	{
		uint32_t commandBufferCount;
		OpenGLCommandBuffer ** pCommandBuffers;
	};
	struct ExecuteCommandsVectorArgs
	{
		std::vector<CommandBuffer *> commandBuffers;
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
	struct UpdateBufferArgs
	{
		GLuint buffer;
		GLintptr offset;
		GLsizeiptr size;
		void * data;
	};
	using RenderCommand = std::variant<BeginRenderPassArgs, BindDescriptorSetArgs, BindIndexBufferArgs, BindPipelineArgs,
									   BindVertexBufferArgs, DrawIndexedArgs, EndRenderPassArgs, ExecuteCommandsArgs,
									   ExecuteCommandsVectorArgs, SetScissorArgs, SetViewportArgs, UpdateBufferArgs>;

	/*
		The list of commands to execute
	*/
	std::vector<RenderCommand> commandList;

	/*
		State trackers
	*/
	size_t indexBufferOffset;
	GLenum indexBufferType;

	virtual void Reset();
};

#endif
