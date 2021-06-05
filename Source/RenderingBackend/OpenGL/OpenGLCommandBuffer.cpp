#ifdef USE_OGL_RENDERER
#include "OpenGLCommandBuffer.h"

#include <assert.h>
#include <gl/glew.h>
#include <unordered_map>

#include "../Renderer.h"
#include "DescriptorSetBindingMap.h"
#include "Logging/Logger.h"
#include "OpenGLConverterFuncs.h"
#include "OpenGLResourceContext.h"
#include "Util/Semaphore.h"

static const auto logger = Logger::Create("OpenGLCommandBuffer");

void OpenGLCommandBuffer::BeginRecording(InheritanceInfo *) {}

void OpenGLCommandBuffer::EndRecording() {}

void OpenGLCommandBuffer::CmdBeginRenderPass(CommandBuffer::RenderPassBeginInfo * pRenderPassBegin,
                                             CommandBuffer::SubpassContents contents)
{
    BeginRenderPassArgs args;
    args.renderPass = pRenderPassBegin->renderPass;
    args.framebuffer = pRenderPassBegin->framebuffer;
    args.clearValueCount = pRenderPassBegin->clearValueCount;
    args.pClearValues = (ClearValue *)allocator->allocate(args.clearValueCount * sizeof(ClearValue const));
    memcpy(args.pClearValues, pRenderPassBegin->pClearValues, args.clearValueCount * sizeof(ClearValue const));
    commandList.push_back(args);
}

void OpenGLCommandBuffer::CmdBindDescriptorSets(PipelineLayoutHandle * layout, uint32_t offset,
                                                std::vector<DescriptorSet *> sets)
{
    auto nativeLayout = (OpenGLPipelineLayoutHandle *)layout;
    std::vector<OpenGLDescriptorSet *> nativeSets(sets.size());
    for (size_t i = 0; i < sets.size(); ++i) {
        nativeSets[i] = (OpenGLDescriptorSet *)sets[i];
    }
    BindDescriptorSetArgs args;
    DescriptorSetBindingMap descriptorSetBindingMap(nativeLayout);

    for (size_t i = 0; i < sets.size(); ++i) {
        auto nativeSet = (OpenGLDescriptorSet *)sets[i];
        for (auto & d : nativeSet->descriptors) {
            auto set = i + offset;
            auto finalBinding = descriptorSetBindingMap.GetBinding(set, d.binding);
            switch (d.type) {
            case DescriptorType::COMBINED_IMAGE_SAMPLER: {
                auto image = std::get<ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor>(d.descriptor);
                auto nativeImageView = (OpenGLImageViewHandle *)image.imageView;
                auto nativeSampler = (OpenGLSamplerHandle *)image.sampler;
                args.images.push_back({.binding = finalBinding,
                                       .image = ((OpenGLImageHandle *)nativeImageView->image)->nativeHandle,
                                       .sampler = nativeSampler->nativeHandle,
                                       .hasSampler = true});
                break;
            }
            case DescriptorType::UNIFORM_BUFFER: {
                auto buf = std::get<0>(d.descriptor);
                args.buffers.push_back({finalBinding,
                                        ((OpenGLBufferHandle *)buf.buffer)->nativeHandle,
                                        (GLintptr)buf.offset,
                                        (GLsizeiptr)buf.range});
                break;
            }
            case DescriptorType::INPUT_ATTACHMENT: {
                auto image = std::get<ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor>(d.descriptor);
                auto nativeImageView = (OpenGLImageViewHandle *)image.imageView;
                args.images.push_back({.binding = finalBinding,
                                       .image = ((OpenGLImageHandle *)nativeImageView->image)->nativeHandle,
                                       .sampler = 0,
                                       .hasSampler = false});
                break;
            }
            default:
                logger.Error("Unknown descriptor type.");
                break;
            }
        }
    }

    commandList.push_back(args);
}

void OpenGLCommandBuffer::CmdBindIndexBuffer(BufferHandle * buffer, size_t offset, CommandBuffer::IndexType indexType)
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
        logger.Error("Unknown index buffer index type");
    }

    commandList.push_back(BindIndexBufferArgs{nativeBuffer->nativeHandle, offset, type});
}

void OpenGLCommandBuffer::CmdBindPipeline(RenderPassHandle::PipelineBindPoint bp, PipelineHandle * handle)
{
    auto nativeHandle = (OpenGLPipelineHandle *)handle;
    commandList.push_back(BindPipelineArgs{nativeHandle->nativeHandle,
                                           ((OpenGLVertexInputStateHandle *)handle->vertexInputState)->nativeHandle,
                                           ToGLCullMode(nativeHandle->rasterizationState.cullMode),
                                           ToGLFrontFace(nativeHandle->rasterizationState.frontFace),
                                           nativeHandle->depthStencil.depthTestEnable,
                                           nativeHandle->depthStencil.depthWriteEnable,
                                           ToGLCompareOp(nativeHandle->depthStencil.depthCompareOp),
                                           ToGLPrimitiveTopology(nativeHandle->inputAssembly.topology)});
}

void OpenGLCommandBuffer::CmdBindVertexBuffer(BufferHandle * buffer, uint32_t binding, size_t offset, uint32_t stride)
{
    auto nativeBuffer = (OpenGLBufferHandle *)buffer;
    assert(nativeBuffer->nativeHandle != 0);

    commandList.push_back(BindVertexBufferArgs{nativeBuffer->nativeHandle, binding, offset, stride});
}

void OpenGLCommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                         int32_t vertexOffset, uint32_t firstInstance)
{
    commandList.push_back(DrawIndexedArgs{
        static_cast<int>(indexCount), firstIndex, static_cast<int>(instanceCount), vertexOffset, firstInstance});
}

void OpenGLCommandBuffer::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                  uint32_t firstInstance)
{
    commandList.push_back(DrawArgs{vertexCount, instanceCount, firstVertex, firstInstance});
}

void OpenGLCommandBuffer::CmdDrawIndirect(BufferHandle * buffer, size_t offset, uint32_t drawCount)
{
    commandList.push_back(DrawIndirectArgs{((OpenGLBufferHandle *)buffer)->nativeHandle, offset, drawCount});
}

void OpenGLCommandBuffer::CmdEndRenderPass() {}

void OpenGLCommandBuffer::CmdExecuteCommands(uint32_t commandBufferCount, CommandBuffer ** pCommandBuffers)
{
    commandList.push_back(ExecuteCommandsArgs{commandBufferCount, (OpenGLCommandBuffer **)pCommandBuffers});
}

void OpenGLCommandBuffer::CmdExecuteCommands(std::vector<CommandBuffer *> && commandBuffers)
{
    commandList.push_back(ExecuteCommandsVectorArgs{std::move(commandBuffers)});
}

void OpenGLCommandBuffer::CmdNextSubpass(SubpassContents subpassContents) {}

void OpenGLCommandBuffer::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount,
                                        CommandBuffer::Rect2D const * pScissors)
{
    auto cmd = SetScissorArgs{firstScissor, static_cast<int>(scissorCount), nullptr};
    cmd.v = (GLint *)allocator->allocate(scissorCount * 4 * sizeof(GLint));
    memcpy((void *)cmd.v, pScissors, scissorCount * 4 * sizeof(GLint));
    commandList.push_back(cmd);
}

void OpenGLCommandBuffer::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount,
                                         CommandBuffer::Viewport const * pViewports)
{
    auto cmd = SetViewportArgs{firstViewport, static_cast<int>(viewportCount), nullptr};
    cmd.v = (GLfloat *)allocator->allocate(viewportCount * 6 * sizeof(GLfloat));
    memcpy((void *)cmd.v, pViewports, viewportCount * 6 * sizeof(GLfloat));
    commandList.push_back(cmd);
}

void OpenGLCommandBuffer::CmdUpdateBuffer(BufferHandle * buffer, size_t offset, size_t size, uint32_t const * pData)
{
    UpdateBufferArgs cmd{((OpenGLBufferHandle *)buffer)->nativeHandle, (GLintptr)offset, (GLsizeiptr)size, nullptr};
    cmd.data = allocator->allocate(size);
    memcpy(cmd.data, pData, size);
    commandList.push_back(cmd);
}

void OpenGLCommandBuffer::Execute(Renderer * renderer, std::vector<SemaphoreHandle *> waitSem,
                                  std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
    for (auto s : waitSem) {
        auto nativeWait = (OpenGLSemaphoreHandle *)s;
        if (nativeWait != nullptr) {
            nativeWait->sem.Wait();
        }
    }
    for (auto & rc : commandList) {
        switch (rc.index()) {
        case RenderCommandType::BEGIN_RENDERPASS: {
            auto args = std::get<BeginRenderPassArgs>(rc);
            glBindFramebuffer(GL_FRAMEBUFFER, ((OpenGLFramebufferHandle *)args.framebuffer)->nativeHandle);
            for (uint32_t i = 0; i < args.clearValueCount; ++i) {
                if (args.pClearValues[i].type == CommandBuffer::ClearValue::Type::COLOR) {
                    auto & color = args.pClearValues[i].color.float32;
                    glClearBufferfv(GL_COLOR, 0, color);
                } else if (args.pClearValues[i].type == CommandBuffer::ClearValue::Type::DEPTH_STENCIL) {
                    auto & depthStencil = args.pClearValues[i].depthStencil;
                    glDepthMask(true);
                    glClearBufferfi(GL_DEPTH_STENCIL, 0, depthStencil.depth, depthStencil.stencil);
                }
            }
            allocator->deallocate((uint8_t *)args.pClearValues, args.clearValueCount * sizeof(ClearValue const));
            break;
        }
        case RenderCommandType::BIND_DESCRIPTOR_SET: {
            auto args = std::get<BindDescriptorSetArgs>(rc);
            for (auto b : args.buffers) {
                glBindBufferRange(GL_UNIFORM_BUFFER, b.binding, b.buffer, b.offset, b.size);
            }
            for (auto i : args.images) {
                glUniform1i(i.binding, i.binding);
                glActiveTexture(GL_TEXTURE0 + i.binding);
                glBindTexture(GL_TEXTURE_2D, i.image);
                if (i.hasSampler) {
                    glBindSampler(i.binding, i.sampler);
                }
            }
            break;
        }
        case RenderCommandType::BIND_INDEX_BUFFER: {
            auto args = std::get<BindIndexBufferArgs>(rc);
            assert(args.buffer != 0);
            assert(args.indexType == GL_UNSIGNED_SHORT || args.indexType == GL_UNSIGNED_INT);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, args.buffer);
            indexBufferOffset = args.offset;
            indexBufferType = args.indexType;
            break;
        }
        case RenderCommandType::BIND_PIPELINE: {
            auto args = std::get<BindPipelineArgs>(rc);
            assert(args.program != 0);
            if (args.cullMode == GL_NONE) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(args.cullMode);
            }
            glFrontFace(args.frontFace);
            glUseProgram(args.program);
            glBindVertexArray(args.vao);
            glDepthMask(args.depthWriteEnable);
            if (args.depthTestEnable) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
            glDepthFunc(args.depthFunc);
            primitiveTopology = args.primitiveTopology;
            break;
        }
        case RenderCommandType::BIND_VERTEX_BUFFER: {
            auto args = std::get<BindVertexBufferArgs>(rc);
            assert(args.buffer != 0);
            glBindVertexBuffer(args.binding, args.buffer, args.offset, args.stride);
            break;
        }
        case RenderCommandType::DRAW: {
            auto args = std::get<DrawArgs>(rc);
            glDrawArraysInstancedBaseInstance(
                primitiveTopology, args.firstVertex, args.vertexCount, args.instanceCount, args.firstInstance);
            break;
        }
        case RenderCommandType::DRAW_INDIRECT: {
            auto args = std::get<DrawIndirectArgs>(rc);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, args.buffer);
            glMultiDrawArraysIndirect(primitiveTopology, (void *)args.offset, (GLsizei)args.drawCount, (GLsizei)16);
            break;
        }
        case RenderCommandType::DRAW_INDEXED: {
            auto args = std::get<DrawIndexedArgs>(rc);
            assert(indexBufferType == GL_UNSIGNED_SHORT || indexBufferType == GL_UNSIGNED_INT);
            auto elementSize = indexBufferType == GL_UNSIGNED_SHORT ? sizeof(uint16_t) : sizeof(uint32_t);
            glDrawElementsInstancedBaseVertex(primitiveTopology,
                                              args.count,
                                              indexBufferType,
                                              (void *)(args.indices * elementSize),
                                              args.primcount,
                                              args.basevertex);
            break;
        }
        case RenderCommandType::EXECUTE_COMMANDS: {
            auto args = std::get<ExecuteCommandsArgs>(rc);
            assert(args.pCommandBuffers != nullptr);
            for (uint32_t i = 0; i < args.commandBufferCount; ++i) {
                args.pCommandBuffers[i]->Execute(renderer, {}, {}, nullptr);
            }
            break;
        }
        case RenderCommandType::EXECUTE_COMMANDS_VECTOR: {
            auto args = std::move(std::get<ExecuteCommandsVectorArgs>(rc));

            for (auto & ctx : args.commandBuffers) {
                ((OpenGLCommandBuffer *)ctx)->Execute(renderer, {}, {}, nullptr);
            }
            break;
        }
        case RenderCommandType::SET_SCISSOR: {
            auto args = std::get<SetScissorArgs>(rc);
            assert(args.v);
            glScissorArrayv(args.first, args.count, args.v);
            allocator->deallocate((uint8_t *)args.v, args.count * 4 * sizeof(GLint));
            break;
        }
        case RenderCommandType::SET_VIEWPORT: {
            auto args = std::get<SetViewportArgs>(rc);
            assert(args.v);
            glViewportArrayv(args.first, args.count, args.v);
            allocator->deallocate((uint8_t *)args.v, args.count * 6 * sizeof(GLfloat));
            break;
        }
        case RenderCommandType::UPDATE_BUFFER: {
            auto args = std::get<UpdateBufferArgs>(rc);
            glNamedBufferSubData(args.buffer, args.offset, args.size, args.data);
            allocator->deallocate((uint8_t *)args.data, args.size);
            break;
        }
        default:
            logger.Error("Unknown render command type {}", rc.index());
            break;
        }
        auto err = glGetError();
        if (err) {
            logger.Error("commandList error {}, commandType {}", err, rc.index());
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

bool OpenGLFenceHandle::Wait(uint64_t timeOut)
{
    sem.Wait();
    return true;
}

#endif
