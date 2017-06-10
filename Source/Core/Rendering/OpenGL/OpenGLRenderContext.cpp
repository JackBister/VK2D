#include "Core/Rendering/OpenGL/OpenGLRenderContext.h"

#include <assert.h>
#include <gl/glew.h>

template <typename T>
static typename std::underlying_type<T>::type ToUnderlyingType(T in)
{
	return static_cast<typename std::underlying_type<T>::type>(in);
}

static GLint AddressModeGL(AddressMode addressMode)
{
	switch (addressMode) {
		case AddressMode::REPEAT:
			return GL_REPEAT;
		case AddressMode::MIRRORED_REPEAT:
			return GL_MIRRORED_REPEAT;
		case AddressMode::CLAMP_TO_EDGE:
			return GL_CLAMP_TO_EDGE;
		case AddressMode::CLAMP_TO_BORDER:
			return GL_CLAMP_TO_BORDER;
		case AddressMode::MIRROR_CLAMP_TO_EDGE:
			return GL_MIRROR_CLAMP_TO_EDGE;
		default:
			printf("[ERROR] Unrecognized sampler address mode: %d\n", ToUnderlyingType(addressMode));
			return GL_REPEAT;
	}
}

static GLint FilterGL(Filter filter)
{
	switch (filter) {
	case Filter::LINEAR:
		return GL_LINEAR;
	case Filter::NEAREST:
		return GL_NEAREST;
	default:
		printf("[ERROR] Unrecognized sampler filter: %d\n", ToUnderlyingType(filter));
		return GL_LINEAR;
	}
}

static GLint FormatGL(Format format)
{
	switch (format) {
	case Format::R8:
		return GL_R;
	case Format::RG8:
		return GL_RG;
	case Format::RGB8:
		return GL_RGB;
	case Format::RGBA8:
		return GL_RGBA;
	default:
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		return GL_RGBA;
	}
}

static GLint InternalFormatGL(Format format)
{
	switch (format) {
	case Format::R8:
		return GL_R8;
	case Format::RG8:
		return GL_RG8;
	case Format::RGB8:
		return GL_RGB8;
	case Format::RGBA8:
		return GL_RGBA8;
	default:
		printf("[ERROR] Unrecognized image format: %d\n", ToUnderlyingType(format));
		return GL_RGBA8;
	}
}

BufferHandle * OpenGLResourceContext::CreateBuffer(BufferCreateInfo bc)
{
	auto ret = (OpenGLBufferHandle *)allocator.allocate(sizeof(OpenGLBufferHandle));
	glCreateBuffers(1, &ret->nativeHandle);
	glNamedBufferData(ret->nativeHandle, bc.size, nullptr, GL_STATIC_DRAW);
	return ret;
}

void OpenGLResourceContext::DestroyBuffer(BufferHandle * handle)
{
	glDeleteBuffers(1, &((OpenGLBufferHandle *)handle)->nativeHandle);
}

uint8_t * OpenGLResourceContext::MapBuffer(BufferHandle * handle, size_t offset, size_t size)
{
	return (uint8_t *)glMapNamedBufferRange(((OpenGLBufferHandle *)handle)->nativeHandle, offset, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
}

void OpenGLResourceContext::UnmapBuffer(BufferHandle * handle)
{
	glUnmapNamedBuffer(((OpenGLBufferHandle *)handle)->nativeHandle);
}

ImageHandle * OpenGLResourceContext::CreateImage(ImageCreateInfo ic)
{
	auto ret = (OpenGLImageHandle *)allocator.allocate(sizeof(OpenGLImageHandle));
	ret->format = ic.format;
	ret->type = ic.type;
	ret->width = ic.width;
	ret->height = ic.height;
	ret->depth = ic.depth;
	glGenTextures(1, &ret->nativeHandle);
	switch (ic.type) {
	case ImageHandle::Type::TYPE_1D:
		ret->height = 1;
		ret->depth = 1;
		glBindTexture(GL_TEXTURE_1D, ret->nativeHandle);
		glTexStorage1D(GL_TEXTURE_1D, ic.mipLevels, InternalFormatGL(ic.format), ic.width);
		break;
	case ImageHandle::Type::TYPE_2D:
		ret->depth = 1;
		glBindTexture(GL_TEXTURE_2D, ret->nativeHandle);
		glTexStorage2D(GL_TEXTURE_2D, ic.mipLevels, InternalFormatGL(ic.format), ic.width, ic.height);	
		break;
	}

	//TODO:

	return ret;
}

void OpenGLResourceContext::DestroyImage(ImageHandle * handle)
{
	glDeleteTextures(1, &((OpenGLImageHandle *)handle)->nativeHandle);
	allocator.destroy(handle);
}

void OpenGLResourceContext::ImageData(ImageHandle * handle, const std::vector<uint8_t>& data)
{
	switch (handle->type) {
	case ImageHandle::Type::TYPE_1D:
		glBindTexture(GL_TEXTURE_1D, ((OpenGLImageHandle *)handle)->nativeHandle);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, handle->width, FormatGL(handle->format), GL_UNSIGNED_BYTE, data.data());
		break;
	case ImageHandle::Type::TYPE_2D:
		glBindTexture(GL_TEXTURE_2D, ((OpenGLImageHandle *)handle)->nativeHandle);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, handle->width, handle->height, FormatGL(handle->format), GL_UNSIGNED_BYTE, data.data());
		break;
	}
}

RenderPassHandle * OpenGLResourceContext::CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo ci)
{
	auto ret = (OpenGLRenderPassHandle *)allocator.allocate(sizeof(OpenGLRenderPassHandle));

	return ret;
}

void OpenGLResourceContext::DestroyRenderPass(RenderPassHandle * handle)
{
	allocator.destroy(handle);
}

ImageViewHandle * OpenGLResourceContext::CreateImageView(ResourceCreationContext::ImageViewCreateInfo ci)
{
	auto ret = (OpenGLImageViewHandle *)allocator.allocate(sizeof(OpenGLImageViewHandle));
	ret->image = ci.image;
	ret->subresourceRange = ci.subresourceRange;
	return ret;
}

void OpenGLResourceContext::DestroyImageView(ImageViewHandle * handle)
{
	allocator.destroy(handle);
}

FramebufferHandle * OpenGLResourceContext::CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo ci)
{
	assert(ci.attachmentCount != 0);
	assert(ci.pAttachments != nullptr);
	assert(ci.width != 0);
	assert(ci.height != 0);
	auto ret = (OpenGLFramebufferHandle *)allocator.allocate(sizeof(OpenGLFramebufferHandle));
	ret->attachmentCount = ci.attachmentCount;
	ret->pAttachments = ci.pAttachments;
	ret->width = ci.width;
	ret->height = ci.height;
	ret->layers = ci.layers;
	glCreateFramebuffers(1, &ret->nativeHandle);
	//Arithmetic on enums = ugly, but option is 16-way switch case.
	GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
	for (uint32_t i = 0; i < ci.attachmentCount; ++i) {
		assert(ci.pAttachments[i]);
		auto tex = ((OpenGLImageHandle *)ci.pAttachments[i]->image);
		if (ci.pAttachments[i]->subresourceRange.aspectMask & ImageViewHandle::COLOR_BIT) {
			assert(colorAttachment <= GL_COLOR_ATTACHMENT12);
			glNamedFramebufferTexture(ret->nativeHandle, colorAttachment, tex->nativeHandle, 0);
			colorAttachment++;
		} else if (ci.pAttachments[i]->subresourceRange.aspectMask & ImageViewHandle::DEPTH_BIT) {
			//An attachment can be depth-stencil, which combines the two bits.
			if (ci.pAttachments[i]->subresourceRange.aspectMask & ImageViewHandle::STENCIL_BIT) {
				glNamedFramebufferTexture(ret->nativeHandle, GL_DEPTH_STENCIL_ATTACHMENT, tex->nativeHandle, 0);
			} else {
				glNamedFramebufferTexture(ret->nativeHandle, GL_DEPTH_ATTACHMENT, tex->nativeHandle, 0);
			}
		} else if (ci.pAttachments[i]->subresourceRange.aspectMask & ImageViewHandle::STENCIL_BIT) {
			glNamedFramebufferTexture(ret->nativeHandle, GL_STENCIL_ATTACHMENT, tex->nativeHandle, 0);
		} else {
			printf("[ERROR] Unknown image aspect bit in %ud.", ci.pAttachments[i]->subresourceRange);
		}
	}
	return ret;
}

void OpenGLResourceContext::DestroyFramebuffer(FramebufferHandle * handle)
{
	assert(handle != nullptr);
	assert(((OpenGLFramebufferHandle *)handle)->nativeHandle != 0);
	glDeleteFramebuffers(1, &((OpenGLFramebufferHandle *)handle)->nativeHandle);
	allocator.destroy(handle);
}

PipelineHandle * OpenGLResourceContext::CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo ci)
{
	assert(ci.stageCount != 0);
	assert(ci.pStages != nullptr);
	auto ret = (OpenGLPipelineHandle *)allocator.allocate(sizeof(OpenGLPipelineHandle));
	
	ret->nativeHandle = glCreateProgram();

	for (uint32_t i = 0; i < ci.stageCount; ++i) {
		auto stageInfo = ci.pStages[i];
		assert(stageInfo.module != nullptr);
		glAttachShader(ret->nativeHandle, ((OpenGLShaderModuleHandle *)stageInfo.module)->nativeHandle);
		GLenum glErr = GL_NO_ERROR;
		if ((glErr = glGetError()) != GL_NO_ERROR) {
			printf("[ERROR] Attaching shader with name %s %d\n", stageInfo.name.c_str(), glErr);
		}
	}

	glLinkProgram(ret->nativeHandle);
	GLenum glErr = GL_NO_ERROR;
	if ((glErr = glGetError()) != GL_NO_ERROR) {
		printf("[ERROR] Linking GL program.\n");
	}

	ret->vertexInputState = ci.vertexInputState;
	ret->descriptorSetLayout = ci.descriptorSetLayout;

	return ret;
}

void OpenGLResourceContext::DestroyPipeline(PipelineHandle * handle)
{
	assert(handle != nullptr);
	assert(((OpenGLPipelineHandle *)handle)->nativeHandle != 0);

	glDeleteProgram(((OpenGLPipelineHandle *)handle)->nativeHandle);
	allocator.destroy(handle);
}

ShaderModuleHandle * OpenGLResourceContext::CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo ci)
{
	assert(ci.codeSize != 0);
	assert(ci.pCode != nullptr);
	auto ret = (OpenGLShaderModuleHandle *)allocator.allocate(sizeof(OpenGLShaderModuleHandle));
	GLenum type = GL_VERTEX_SHADER;
	switch (ci.type) {
	case ShaderModuleCreateInfo::Type::VERTEX_SHADER:
		type = GL_VERTEX_SHADER;
		break;
	case ShaderModuleCreateInfo::Type::FRAGMENT_SHADER:
		type = GL_FRAGMENT_SHADER;
		break;
	default:
		printf("[ERROR] Unknown shader module type %d.\n", ToUnderlyingType(ci.type));
	}
	ret->nativeHandle = glCreateShader(type);
	const GLchar * src = (GLchar *)ci.pCode;
	glShaderSource(ret->nativeHandle, 1, &src, nullptr);
	printf("post shadersource err %d\n", glGetError());
	glCompileShader(ret->nativeHandle);
	GLint compileOk;
	glGetShaderiv(ret->nativeHandle, GL_COMPILE_STATUS, &compileOk);
	if (compileOk != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(ret->nativeHandle, 1024, &log_length, message);
		printf("Shader compile failed\n%s\n", message);
		glDeleteShader(ret->nativeHandle);
		allocator.destroy(ret);
		return nullptr;
	}
	return ret;
}

void OpenGLResourceContext::DestroyShaderModule(ShaderModuleHandle * handle)
{
	auto nativeShader = (OpenGLShaderModuleHandle *)handle;
	glDeleteShader(nativeShader->nativeHandle);
	allocator.destroy(handle);
}

SamplerHandle * OpenGLResourceContext::CreateSampler(ResourceCreationContext::SamplerCreateInfo ci)
{
	auto ret = (OpenGLSamplerHandle *)allocator.allocate(sizeof(OpenGLSamplerHandle));
	glCreateSamplers(1, &ret->nativeHandle);

	glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_WRAP_S, AddressModeGL(ci.addressModeU));
	glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_WRAP_T, AddressModeGL(ci.addressModeV));
	glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_MAG_FILTER, FilterGL(ci.magFilter));
	glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_MIN_FILTER, FilterGL(ci.minFilter));
	return ret;
}

void OpenGLResourceContext::DestroySampler(SamplerHandle * handle)
{
	assert(handle != nullptr);
	auto handleGL = (OpenGLSamplerHandle *)handle;
	assert(handleGL->nativeHandle != 0);
	glDeleteSamplers(1, &handleGL->nativeHandle);
	allocator.destroy(handleGL);
}

DescriptorSetLayoutHandle * OpenGLResourceContext::CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo ci)
{
	auto ret = (OpenGLDescriptorSetLayoutHandle *)allocator.allocate(sizeof(OpenGLDescriptorSetLayoutHandle));
	ret->bindings = std::vector<ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding>(ci.bindingCount);
	memcpy(&ret->bindings[0], ci.pBinding, sizeof(ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding) * ci.bindingCount);
	return ret;
}

void OpenGLResourceContext::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle * handle)
{
	assert(handle != nullptr);
	allocator.destroy(handle);
}

VertexInputStateHandle * OpenGLResourceContext::CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo ci)
{
	auto ret = (OpenGLVertexInputStateHandle *)allocator.allocate(sizeof(OpenGLVertexInputStateHandle));
	glCreateVertexArrays(1, &ret->nativeHandle);
	for (uint32_t i = 0; i < ci.vertexAttributeDescriptionCount; ++i) {
		auto attributeDescription = ci.pVertexAttributeDescriptions[i];
		GLenum type = GL_FLOAT;
		switch (attributeDescription.type) {
		case VertexComponentType::BYTE:
			type = GL_BYTE;
			break;
		case VertexComponentType::SHORT:
			type = GL_SHORT;
			break;
		case VertexComponentType::INT:
			type = GL_INT;
			break;
		case VertexComponentType::FLOAT:
			type = GL_FLOAT;
			break;
		case VertexComponentType::HALF_FLOAT:
			type = GL_HALF_FLOAT;
			break;
		case VertexComponentType::DOUBLE:
			type = GL_DOUBLE;
			break;
		case VertexComponentType::UBYTE:
			type = GL_UNSIGNED_BYTE;
			break;
		case VertexComponentType::USHORT:
			type = GL_UNSIGNED_SHORT;
			break;
		case VertexComponentType::UINT:
			type = GL_UNSIGNED_INT;
			break;
		default:
			printf("[ERROR] Unknown vertex input type %d\n", ToUnderlyingType(attributeDescription.type));
		}

		glVertexArrayAttribFormat(ret->nativeHandle, attributeDescription.location, attributeDescription.size, type, attributeDescription.normalized, attributeDescription.offset);
		glVertexArrayAttribBinding(ret->nativeHandle, attributeDescription.location, attributeDescription.binding);
		glEnableVertexArrayAttrib(ret->nativeHandle, attributeDescription.location);
	}
	return ret;
}

void OpenGLResourceContext::DestroyVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo * handle)
{
	assert(handle != nullptr);
	auto nativeHandle = (OpenGLVertexInputStateHandle *)handle;
	assert(nativeHandle->nativeHandle != 0);
	glDeleteVertexArrays(1, &nativeHandle->nativeHandle);
	allocator.destroy(handle);
}

DescriptorSet * OpenGLResourceContext::CreateDescriptorSet(DescriptorSetCreateInfo ci)
{
	auto mem = allocator.allocate(sizeof(OpenGLDescriptorSet));
	auto ret = (OpenGLDescriptorSet *)new (mem) OpenGLDescriptorSet();
	ret->descriptors = std::vector<ResourceCreationContext::DescriptorSetCreateInfo::Descriptor>(ci.descriptorCount);
	memcpy(&(ret->descriptors[0]), ci.descriptors, ci.descriptorCount * sizeof(ResourceCreationContext::DescriptorSetCreateInfo::Descriptor));
	return ret;
}

void OpenGLResourceContext::DestroyDescriptorSet(DescriptorSet * set)
{
	assert(set != nullptr);
	allocator.destroy(set);
}

void OpenGLRenderCommandContext::CmdBeginRenderPass(RenderCommandContext::RenderPassBeginInfo *pRenderPassBegin, RenderCommandContext::SubpassContents contents)
{
	BeginRenderPassArgs args;
	args.renderPass = pRenderPassBegin->renderPass;
	args.framebuffer = pRenderPassBegin->framebuffer;
	args.clearValueCount = pRenderPassBegin->clearValueCount;
	args.pClearValues = (ClearValue *)allocator->allocate(args.clearValueCount * sizeof(const ClearValue));
	memcpy(args.pClearValues, pRenderPassBegin->pClearValues, args.clearValueCount * sizeof(const ClearValue));
	commandList.push_back(args);
}

void OpenGLRenderCommandContext::CmdBindDescriptorSet(DescriptorSet * set)
{
	auto nativeSet = (OpenGLDescriptorSet *)set;
	BindDescriptorSetArgs args;
	for (auto& d : nativeSet->descriptors) {
		switch (d.type) {
		case DescriptorType::SAMPLER:
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

void OpenGLRenderCommandContext::CmdBindIndexBuffer(BufferHandle *buffer, size_t offset, RenderCommandContext::IndexType indexType)
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

void OpenGLRenderCommandContext::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
	auto nativeBuffer = (OpenGLBufferHandle *)buffer;
	assert(nativeBuffer->nativeHandle != 0);

	commandList.push_back(BindVertexBufferArgs{nativeBuffer->nativeHandle, binding, offset, stride});
}

void OpenGLRenderCommandContext::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset)
{
	commandList.push_back(DrawIndexedArgs{static_cast<int>(indexCount), (const void *)firstIndex, static_cast<int>(instanceCount)});
}

void OpenGLRenderCommandContext::CmdEndRenderPass()
{
}

void OpenGLRenderCommandContext::CmdExecuteCommands(uint32_t commandBufferCount, RenderCommandContext ** pCommandBuffers)
{
	commandList.push_back(ExecuteCommandsArgs{ commandBufferCount, (OpenGLRenderCommandContext **)pCommandBuffers });
}

void OpenGLRenderCommandContext::CmdExecuteCommands(std::vector<std::unique_ptr<RenderCommandContext>>&& commandBuffers)
{
	commandList.push_back(ExecuteCommandsVectorArgs{ std::move(commandBuffers) });
}

void OpenGLRenderCommandContext::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const RenderCommandContext::Rect2D *pScissors)
{
	auto cmd = SetScissorArgs{firstScissor, static_cast<int>(scissorCount), nullptr};
	cmd.v = (GLint *)allocator->allocate(scissorCount * 4 * sizeof(GLint));
	memcpy((void *)cmd.v, pScissors, scissorCount * 4 * sizeof(GLint));
	commandList.push_back(cmd);
}

void OpenGLRenderCommandContext::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const RenderCommandContext::Viewport *pViewports)
{
	auto cmd = SetViewportArgs{firstViewport, static_cast<int>(viewportCount), nullptr};
	cmd.v = (GLfloat *)allocator->allocate(viewportCount * 6 * sizeof(GLfloat));
	memcpy((void *)cmd.v, pViewports, viewportCount * 6 * sizeof(GLfloat));
	commandList.push_back(cmd);
}

void OpenGLRenderCommandContext::CmdSwapWindow()
{
	commandList.push_back(SwapWindowArgs{});
}

void OpenGLRenderCommandContext::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, const uint32_t * pData)
{
	commandList.push_back(UpdateBufferArgs{
		((OpenGLBufferHandle *)buffer)->nativeHandle,
		(GLintptr)offset,
		(GLsizeiptr)size,
		pData
	});
}

void OpenGLRenderCommandContext::Execute()
{
	Execute(nullptr);
}

void OpenGLRenderCommandContext::Execute(SDL_Window * win)
{
	for (auto& rc : commandList) {
		switch (rc.index()) {
		case RenderCommandType::BEGIN_RENDERPASS:
		{
			auto args = std::get<BeginRenderPassArgs>(rc);
			glBindFramebuffer(GL_FRAMEBUFFER, ((OpenGLFramebufferHandle *)args.framebuffer)->nativeHandle);
			for (uint32_t i = 0; i < args.clearValueCount; ++i) {
				if (args.pClearValues[i].type == RenderCommandContext::ClearValue::Type::COLOR) {
					auto& color = args.pClearValues[i].color.float32;
					glClearColor(color[0], color[1], color[2], color[3]);
					glClear(GL_COLOR_BUFFER_BIT);
				} else if (args.pClearValues[i].type == RenderCommandContext::ClearValue::Type::DEPTH_STENCIL) {
					auto& depthStencil = args.pClearValues[i].depthStencil;
					glClearDepthf(depthStencil.depth);
					glClearStencil(depthStencil.stencil);
					glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				}
			}
			allocator->deallocate((uint8_t *)args.pClearValues, args.clearValueCount * sizeof(const ClearValue));
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
		case RenderCommandType::BIND_UNIFORM_BUFFER:
		{
			auto args = std::get<BindUniformBufferArgs>(rc);
			glBindBufferRange(GL_UNIFORM_BUFFER, args.index, args.buffer, args.offset, args.size);
			break;
		}
		case RenderCommandType::BIND_UNIFORM_IMAGE:
		{
			//TODO:
			auto args = std::get<BindUniformImageArgs>(rc);
			glActiveTexture(GL_TEXTURE0 + args.binding);
			glBindTexture(GL_TEXTURE_2D, args.image);
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
				args.pCommandBuffers[i]->Execute();
			}
			break;
		}
		case RenderCommandType::EXECUTE_COMMANDS_VECTOR:
		{
			auto args = std::move(std::get<ExecuteCommandsVectorArgs>(rc));

			for (auto& ctx : args.commandBuffers) {
				((OpenGLRenderCommandContext *)ctx.get())->Execute();
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
		case RenderCommandType::SWAP_WINDOW:
		{
			if (win != nullptr) {
				SDL_GL_SwapWindow(win);
			}
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
}

void OpenGLRenderCommandContext::CmdBindUniformBuffer(uint32_t binding, BufferHandle * buffer, uint32_t offset, uint32_t size)
{
	commandList.push_back(BindUniformBufferArgs{
		binding,
		((OpenGLBufferHandle *)buffer)->nativeHandle,
		offset,
		size
	});
}

void OpenGLRenderCommandContext::CmdBindUniformImage(uint32_t binding, ImageHandle * img)
{
	commandList.push_back(BindUniformImageArgs{
		binding,
		((OpenGLImageHandle *)img)->nativeHandle
	});
}

void OpenGLRenderCommandContext::CmdBindPipeline(RenderPassHandle::PipelineBindPoint bp, PipelineHandle * handle)
{
	commandList.push_back(BindPipelineArgs{
		((OpenGLPipelineHandle *)handle)->nativeHandle,
		((OpenGLVertexInputStateHandle *)handle->vertexInputState)->nativeHandle
	});
}
