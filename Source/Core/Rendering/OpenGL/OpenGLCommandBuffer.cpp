#ifdef USE_OGL_RENDERER
#include "Core/Rendering/OpenGL/OpenGLCommandBuffer.h"

#include <assert.h>
#include <gl/glew.h>

#include "Core/Rendering/OpenGL/OpenGLConverterFuncs.h"
#include "Core/Rendering/OpenGL/OpenGLResourceContext.h"
#include "Core/Rendering/Renderer.h"
#include "Core/Semaphore.h"

void OpenGLCommandBuffer::BeginRecording(InheritanceInfo *)
{
}

void OpenGLCommandBuffer::EndRecording()
{
}

void OpenGLCommandBuffer::CmdBeginRenderPass(CommandBuffer::RenderPassBeginInfo *pRenderPassBegin, CommandBuffer::SubpassContents contents)
{
	BeginRenderPassArgs args;
	args.renderPass = pRenderPassBegin->renderPass;
	args.framebuffer = pRenderPassBegin->framebuffer;
	args.clearValueCount = pRenderPassBegin->clearValueCount;
	args.pClearValues = (ClearValue *)allocator->allocate(args.clearValueCount * sizeof(ClearValue const));
	memcpy(args.pClearValues, pRenderPassBegin->pClearValues, args.clearValueCount * sizeof(ClearValue const));
	commandList.push_back(args);
}

void OpenGLCommandBuffer::CmdBindDescriptorSet(DescriptorSet * set)
{
	auto nativeSet = (OpenGLDescriptorSet *)set;
	BindDescriptorSetArgs args;
	for (auto& d : nativeSet->descriptors) {
		switch (d.type) {
		case DescriptorType::COMBINED_IMAGE_SAMPLER:
			args.images.push_back({
				d.binding,
				((OpenGLImageHandle *)std::get<1>(d.descriptor).img)->nativeHandle
			});
			break;
		case DescriptorType::UNIFORM_BUFFER:
			auto buf = std::get<0>(d.descriptor);
			args.buffers.push_back({
				d.binding,
				((OpenGLBufferHandle *)buf.buffer)->nativeHandle,
				(GLintptr)buf.offset,
				(GLsizeiptr)buf.range
			});
			break;
		default:
			printf("[ERROR] Unknown descriptor type.\n");
			break;
		}
	}
	commandList.push_back(args);
}

void OpenGLCommandBuffer::CmdBindIndexBuffer(BufferHandle *buffer, size_t offset, CommandBuffer::IndexType indexType)
{
	auto nativeBuffer = (OpenGLBufferHandle *)buffer;
	assert(nativeBuffer->nativeHandle != 0);

	GLenum type = GL_UNSIGNED_INT;
	switch (indexType) {
	case IndexType::UINT16:
		type = GL_UNSIGNED_SHORT;
		break;
	case IndexType::UINT32:
		type = GL_UNSIGNED_INT;
		break;
	default:
		printf("[ERROR] Unknown index buffer index type");
	}

	commandList.push_back(BindIndexBufferArgs{nativeBuffer->nativeHandle, offset, type});
}

void OpenGLCommandBuffer::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
	auto nativeBuffer = (OpenGLBufferHandle *)buffer;
	assert(nativeBuffer->nativeHandle != 0);

	commandList.push_back(BindVertexBufferArgs{nativeBuffer->nativeHandle, binding, offset, stride});
}

void OpenGLCommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset)
{
	commandList.push_back(DrawIndexedArgs{static_cast<int>(indexCount), (void const *)firstIndex, static_cast<int>(instanceCount)});
}

void OpenGLCommandBuffer::CmdEndRenderPass()
{
}

void OpenGLCommandBuffer::CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers)
{
	commandList.push_back(ExecuteCommandsArgs{ commandBufferCount, (OpenGLCommandBuffer **)pCommandBuffers });
}

void OpenGLCommandBuffer::CmdExecuteCommands(std::vector<CommandBuffer *>&& commandBuffers)
{
	commandList.push_back(ExecuteCommandsVectorArgs{ std::move(commandBuffers) });
}

void OpenGLCommandBuffer::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, CommandBuffer::Rect2D const * pScissors)
{
	auto cmd = SetScissorArgs{firstScissor, static_cast<int>(scissorCount), nullptr};
	cmd.v = (GLint *)allocator->allocate(scissorCount * 4 * sizeof(GLint));
	memcpy((void *)cmd.v, pScissors, scissorCount * 4 * sizeof(GLint));
	commandList.push_back(cmd);
}

void OpenGLCommandBuffer::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, CommandBuffer::Viewport const * pViewports)
{
	auto cmd = SetViewportArgs{firstViewport, static_cast<int>(viewportCount), nullptr};
	cmd.v = (GLfloat *)allocator->allocate(viewportCount * 6 * sizeof(GLfloat));
	memcpy((void *)cmd.v, pViewports, viewportCount * 6 * sizeof(GLfloat));
	commandList.push_back(cmd);
}

void OpenGLCommandBuffer::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData)
{
	UpdateBufferArgs cmd{
		((OpenGLBufferHandle *)buffer)->nativeHandle,
		(GLintptr)offset,
		(GLsizeiptr)size,
		nullptr
	};
	cmd.data = allocator->allocate(size);
	memcpy(cmd.data, pData, size);
	commandList.push_back(cmd);
}

void OpenGLCommandBuffer::Execute(Renderer * renderer, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
	for (auto s : waitSem) {
		auto nativeWait = (OpenGLSemaphoreHandle *)s;
		if (nativeWait != nullptr) {
			nativeWait->sem.Wait();
		}
	}
	for (auto& rc : commandList) {
		switch (rc.index()) {
		case RenderCommandType::BEGIN_RENDERPASS:
		{
			auto args = std::get<BeginRenderPassArgs>(rc);
			glBindFramebuffer(GL_FRAMEBUFFER, ((OpenGLFramebufferHandle *)args.framebuffer)->nativeHandle);
			for (uint32_t i = 0; i < args.clearValueCount; ++i) {
				if (args.pClearValues[i].type == CommandBuffer::ClearValue::Type::COLOR) {
					auto& color = args.pClearValues[i].color.float32;
					glClearColor(color[0], color[1], color[2], color[3]);
					glClear(GL_COLOR_BUFFER_BIT);
				} else if (args.pClearValues[i].type == CommandBuffer::ClearValue::Type::DEPTH_STENCIL) {
					auto& depthStencil = args.pClearValues[i].depthStencil;
					glClearDepthf(depthStencil.depth);
					glClearStencil(depthStencil.stencil);
					glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				}
			}
			allocator->deallocate((uint8_t *)args.pClearValues, args.clearValueCount * sizeof(ClearValue const));
			break;
		}
		case RenderCommandType::BIND_DESCRIPTOR_SET:
		{
			auto args = std::get<BindDescriptorSetArgs>(rc);
			for (auto b : args.buffers) {
				glBindBufferRange(GL_UNIFORM_BUFFER, b.binding, b.buffer, b.offset, b.size);
			}
			for (auto i : args.images) {
				glActiveTexture(GL_TEXTURE0 + i.binding);
				glBindTexture(GL_TEXTURE_2D, i.image);
			}
			break;
		}
		case RenderCommandType::BIND_INDEX_BUFFER:
		{
			auto args = std::get<BindIndexBufferArgs>(rc);
			assert(args.buffer != 0);
			assert(args.indexType == GL_UNSIGNED_SHORT || args.indexType == GL_UNSIGNED_INT);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, args.buffer);
			indexBufferOffset = args.offset;
			indexBufferType = args.indexType;
			break;
		}
		case RenderCommandType::BIND_PIPELINE:
		{
			auto args = std::get<BindPipelineArgs>(rc);
			assert(args.program != 0);
			glUseProgram(args.program);
			glBindVertexArray(args.vao);
			break;
		}
		case RenderCommandType::BIND_VERTEX_BUFFER:
		{
			auto args = std::get<BindVertexBufferArgs>(rc);
			assert(args.buffer != 0);
			glBindVertexBuffer(args.binding, args.buffer, args.offset, args.stride);
			break;
		}
		case RenderCommandType::DRAW_INDEXED:
		{
			auto args = std::get<DrawIndexedArgs>(rc);
			assert(indexBufferType == GL_UNSIGNED_SHORT || indexBufferType == GL_UNSIGNED_INT);
			glDrawElementsInstanced(GL_TRIANGLES, args.count, indexBufferType, args.indices, args.primcount);
			break;
		}
		case RenderCommandType::EXECUTE_COMMANDS:
		{
			auto args = std::get<ExecuteCommandsArgs>(rc);
			assert(args.pCommandBuffers != nullptr);
			for (uint32_t i = 0; i < args.commandBufferCount; ++i) {
				args.pCommandBuffers[i]->Execute(renderer, {}, {}, nullptr);
			}
			break;
		}
		case RenderCommandType::EXECUTE_COMMANDS_VECTOR:
		{
			auto args = std::move(std::get<ExecuteCommandsVectorArgs>(rc));

			for (auto& ctx : args.commandBuffers) {
				((OpenGLCommandBuffer *)ctx)->Execute(renderer, {}, {}, nullptr);
			}
			break;
		}
		case RenderCommandType::SET_SCISSOR:
		{
			auto args = std::get<SetScissorArgs>(rc);
			assert(args.v);
			glScissorArrayv(args.first, args.count, args.v);
			allocator->deallocate((uint8_t *)args.v, args.count * 4 * sizeof(GLint));
			break;
		}
		case RenderCommandType::SET_VIEWPORT:
		{
			auto args = std::get<SetViewportArgs>(rc);
			assert(args.v);
			glViewportArrayv(args.first, args.count, args.v);
			allocator->deallocate((uint8_t *)args.v, args.count * 6 * sizeof(GLfloat));
			break;
		}
		case RenderCommandType::UPDATE_BUFFER:
		{
			auto args = std::get<UpdateBufferArgs>(rc);
			glNamedBufferSubData(args.buffer, args.offset, args.size, args.data);
			break;
		}
		default:
			printf("[ERROR] Unknown render command type %zd\n", rc.index());
			break;
		}
	}
	commandList.clear();
	for (auto s : signalSem) {
		auto nativeSignal = (OpenGLSemaphoreHandle *)s;
		if (nativeSignal != nullptr) {
			nativeSignal->sem.Signal();
		}
	}
	if (signalFence != nullptr) {
		((OpenGLFenceHandle *)signalFence)->sem.Signal();
	}
}

void OpenGLCommandBuffer::Reset()
{
	commandList.clear();
}

void OpenGLCommandBuffer::CmdBindPipeline(RenderPassHandle::PipelineBindPoint bp, PipelineHandle * handle)
{
	commandList.push_back(BindPipelineArgs{
		((OpenGLPipelineHandle *)handle)->nativeHandle,
		((OpenGLVertexInputStateHandle *)handle->vertexInputState)->nativeHandle
	});
}

bool OpenGLFenceHandle::Wait(uint64_t timeOut)
{
	sem.Wait();
	return true;
}

#endif
