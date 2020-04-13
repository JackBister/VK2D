#if USE_OGL_RENDERER
#include "Core/Rendering/Backend/OpenGL/OpenGLResourceContext.h"

#include <GL/glew.h>
#include <cassert>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLCommandBufferAllocator.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLContextStructs.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLConverterFuncs.h"

static const auto logger = Logger::Create("OpenGLResourceContext");

void OpenGLResourceContext::BufferSubData(BufferHandle * buffer, uint8_t * data, size_t offset, size_t size)
{
    auto nativeHandle = (OpenGLBufferHandle *)buffer;
    glNamedBufferSubData(nativeHandle->nativeHandle, offset, size, &data[0]);
}

BufferHandle * OpenGLResourceContext::CreateBuffer(BufferCreateInfo const & bc)
{
    auto ret = (OpenGLBufferHandle *)allocator.allocate(sizeof(OpenGLBufferHandle));
    GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    if (bc.memoryProperties & MemoryPropertyFlagBits::HOST_COHERENT_BIT) {
        flags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
    }

    glCreateBuffers(1, &ret->nativeHandle);
    glNamedBufferStorage(ret->nativeHandle, bc.size, nullptr, flags);
    ret->usage = bc.usage;
    ret->memoryProperties = bc.memoryProperties;
    return ret;
}

void OpenGLResourceContext::DestroyBuffer(BufferHandle * handle)
{
    glDeleteBuffers(1, &((OpenGLBufferHandle *)handle)->nativeHandle);
}

uint8_t * OpenGLResourceContext::MapBuffer(BufferHandle * handle, size_t offset, size_t size)
{
    auto nativeHandle = (OpenGLBufferHandle *)handle;
    GLbitfield access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    if (nativeHandle->memoryProperties & MemoryPropertyFlagBits::HOST_COHERENT_BIT) {
        access |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
    }
    return (uint8_t *)glMapNamedBufferRange(nativeHandle->nativeHandle, offset, size, access);
}

void OpenGLResourceContext::UnmapBuffer(BufferHandle * handle)
{
    glUnmapNamedBuffer(((OpenGLBufferHandle *)handle)->nativeHandle);
}

ImageHandle * OpenGLResourceContext::CreateImage(ImageCreateInfo const & ic)
{
    auto ret = (OpenGLImageHandle *)allocator.allocate(sizeof(OpenGLImageHandle));
    ret->format = ic.format;
    ret->type = ic.type;
    ret->width = ic.width;
    ret->height = ic.height;
    ret->depth = ic.depth;
    ret->mipLevels = ic.mipLevels;
    switch (ic.type) {
    case ImageHandle::Type::TYPE_1D:
        ret->height = 1;
        ret->depth = 1;
        glCreateTextures(GL_TEXTURE_1D, 1, &ret->nativeHandle);
        //   glTextureStorage1D(ret->nativeHandle, ic.mipLevels, ToGLInternalFormat(ic.format), ic.width);
        break;
    case ImageHandle::Type::TYPE_2D:
        ret->depth = 1;
        glCreateTextures(GL_TEXTURE_2D, 1, &ret->nativeHandle);
        //    glTextureStorage2D(ret->nativeHandle, ic.mipLevels, ToGLInternalFormat(ic.format), ic.width, ic.height);
        break;
    }
    return ret;
}

void OpenGLResourceContext::DestroyImage(ImageHandle * handle)
{
    glDeleteTextures(1, &((OpenGLImageHandle *)handle)->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLImageHandle));
}

void OpenGLResourceContext::AllocateImage(ImageHandle * handle)
{
    auto native = (OpenGLImageHandle *)handle;

    switch (native->type) {
    case ImageHandle::Type::TYPE_1D:
        glTextureStorage1D(native->nativeHandle, native->mipLevels, ToGLInternalFormat(native->format), native->width);
        break;
    case ImageHandle::Type::TYPE_2D:
        glTextureStorage2D(
            native->nativeHandle, native->mipLevels, ToGLInternalFormat(native->format), native->width, native->height);
        break;
    default:
        logger->Errorf("Attempt to AllocateImage with unknown type %d", native->type);
    }
}

void OpenGLResourceContext::ImageData(ImageHandle * handle, std::vector<uint8_t> const & data)
{
    auto native = (OpenGLImageHandle *)handle;
    switch (handle->type) {
    case ImageHandle::Type::TYPE_1D:
        glBindTexture(GL_TEXTURE_1D, native->nativeHandle);
        glTexStorage1D(GL_TEXTURE_1D, native->mipLevels, ToGLInternalFormat(native->format), native->width);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, handle->width, ToGLFormat(handle->format), GL_UNSIGNED_BYTE, data.data());
        break;
    case ImageHandle::Type::TYPE_2D:
        glBindTexture(GL_TEXTURE_2D, native->nativeHandle);
        glTexStorage2D(
            GL_TEXTURE_2D, native->mipLevels, ToGLInternalFormat(native->format), native->width, native->height);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        handle->width,
                        handle->height,
                        ToGLFormat(handle->format),
                        GL_UNSIGNED_BYTE,
                        data.data());
        break;
    }
}

RenderPassHandle * OpenGLResourceContext::CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo const & ci)
{
    auto ret = (OpenGLRenderPassHandle *)allocator.allocate(sizeof(OpenGLRenderPassHandle));
    ret = new (ret) OpenGLRenderPassHandle();
    ret->createInfo = ci;
    return ret;
}

void OpenGLResourceContext::DestroyRenderPass(RenderPassHandle * handle)
{
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLRenderPassHandle));
}

ImageViewHandle * OpenGLResourceContext::CreateImageView(ResourceCreationContext::ImageViewCreateInfo const & ci)
{
    auto ret = (OpenGLImageViewHandle *)allocator.allocate(sizeof(OpenGLImageViewHandle));
    ret->image = ci.image;
    ret->subresourceRange = ci.subresourceRange;
    return ret;
}

void OpenGLResourceContext::DestroyImageView(ImageViewHandle * handle)
{
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLImageViewHandle));
}

FramebufferHandle * OpenGLResourceContext::CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const & ci)
{
    assert(ci.attachments.size() != 0);
    assert(ci.width != 0);
    assert(ci.height != 0);
    auto ret = (OpenGLFramebufferHandle *)allocator.allocate(sizeof(OpenGLFramebufferHandle));
    ret = new (ret) OpenGLFramebufferHandle();
    ret->attachments.resize(ci.attachments.size());
    for (size_t i = 0; i < ci.attachments.size(); ++i) {
        ret->attachments[i] = (OpenGLImageViewHandle *)ci.attachments[i];
    }
    ret->width = ci.width;
    ret->height = ci.height;
    ret->layers = ci.layers;
    glCreateFramebuffers(1, &ret->nativeHandle);
    // Arithmetic on enums = ugly, but option is 16-way switch case.
    GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
    for (uint32_t i = 0; i < ci.attachments.size(); ++i) {
        assert(ret->attachments[i]);
        auto tex = ((OpenGLImageHandle *)ret->attachments[i]->image);
        if (ret->attachments[i]->subresourceRange.aspectMask & ImageViewHandle::COLOR_BIT) {
            assert(colorAttachment <= GL_COLOR_ATTACHMENT12);
            glNamedFramebufferTexture(ret->nativeHandle, colorAttachment, tex->nativeHandle, 0);
            colorAttachment++;
        } else if (ret->attachments[i]->subresourceRange.aspectMask & ImageViewHandle::DEPTH_BIT) {
            // An attachment can be depth-stencil, which combines the two bits.
            if (ret->attachments[i]->subresourceRange.aspectMask & ImageViewHandle::STENCIL_BIT) {
                glNamedFramebufferTexture(ret->nativeHandle, GL_DEPTH_STENCIL_ATTACHMENT, tex->nativeHandle, 0);
            } else {
                glNamedFramebufferTexture(ret->nativeHandle, GL_DEPTH_ATTACHMENT, tex->nativeHandle, 0);
            }
        } else if (ret->attachments[i]->subresourceRange.aspectMask & ImageViewHandle::STENCIL_BIT) {
            glNamedFramebufferTexture(ret->nativeHandle, GL_STENCIL_ATTACHMENT, tex->nativeHandle, 0);
        } else {
            logger->Errorf("Unknown image aspect bit in %ud.", ret->attachments[i]->subresourceRange.aspectMask);
        }
    }
    return ret;
}

void OpenGLResourceContext::DestroyFramebuffer(FramebufferHandle * handle)
{
    assert(handle != nullptr);
    assert(((OpenGLFramebufferHandle *)handle)->nativeHandle != 0);
    glDeleteFramebuffers(1, &((OpenGLFramebufferHandle *)handle)->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLFramebufferHandle));
}

PipelineLayoutHandle * OpenGLResourceContext::CreatePipelineLayout(PipelineLayoutCreateInfo const & ci)
{
    auto ret = (OpenGLPipelineLayoutHandle *)allocator.allocate(sizeof(OpenGLPipelineLayoutHandle));
    ret = new (ret) OpenGLPipelineLayoutHandle();
    ret->descriptorLayouts = std::vector<DescriptorSetLayoutHandle *>();
    ret->descriptorLayouts = ci.setLayouts;
    return ret;
}

void OpenGLResourceContext::DestroyPipelineLayout(PipelineLayoutHandle * layout)
{
    assert(layout != nullptr);
    allocator.deallocate((uint8_t *)layout, sizeof(PipelineLayoutHandle));
}

PipelineHandle *
OpenGLResourceContext::CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const & ci)
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
            logger->Errorf("Error when attaching shader with name %s %d", stageInfo.name.c_str(), glErr);
        }
    }

    glLinkProgram(ret->nativeHandle);
    GLenum glErr = GL_NO_ERROR;
    if ((glErr = glGetError()) != GL_NO_ERROR) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(ret->nativeHandle, 1024, &log_length, message);
        logger->Errorf("Error when linking GL program. %s", message);
    }

    // This is sort of a hack to help map the combination of descriptorset + binding to just a binding
    GLint numUniformBlocks;
    glGetProgramiv(ret->nativeHandle, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
    for (int i = 0; i < numUniformBlocks; ++i) {
        glUniformBlockBinding(ret->nativeHandle, i, i);
    }

    ret->depthStencil = *ci.depthStencil;
    ret->rasterizationState = *ci.rasterizationState;
    ret->vertexInputState = ci.vertexInputState;

    return ret;
}

void OpenGLResourceContext::DestroyPipeline(PipelineHandle * handle)
{
    assert(handle != nullptr);
    assert(((OpenGLPipelineHandle *)handle)->nativeHandle != 0);

    glDeleteProgram(((OpenGLPipelineHandle *)handle)->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLPipelineHandle));
}

ShaderModuleHandle *
OpenGLResourceContext::CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo const & ci)
{
    assert(ci.code.size() > 0);
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
        logger->Errorf("Unknown shader module type %d.", ToUnderlyingType(ci.type));
    }
    ret->nativeHandle = glCreateShader(type);
    GLchar const * src = (GLchar *)&ci.code[0];
    // glShaderSource(ret->nativeHandle, 1, &src, nullptr);
    // glCompileShader(ret->nativeHandle);
    glShaderBinary(1, &ret->nativeHandle, GL_SHADER_BINARY_FORMAT_SPIR_V, src, (GLsizei)ci.code.size() * 4);

    uint32_t specializationConstantIndexes[] = {0};
    uint32_t specializationConstantValues[] = {1};

    glSpecializeShader(ret->nativeHandle, "main", 1, specializationConstantIndexes, specializationConstantValues);
    GLint compileOk;
    glGetShaderiv(ret->nativeHandle, GL_COMPILE_STATUS, &compileOk);
    if (compileOk != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(ret->nativeHandle, 1024, &log_length, message);
        logger->Errorf("Shader compile failed: %s", message);
        glDeleteShader(ret->nativeHandle);
        allocator.deallocate((uint8_t *)ret, sizeof(OpenGLShaderModuleHandle));
        return nullptr;
    }
    return ret;
}

void OpenGLResourceContext::DestroyShaderModule(ShaderModuleHandle * handle)
{
    auto nativeShader = (OpenGLShaderModuleHandle *)handle;
    glDeleteShader(nativeShader->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLShaderModuleHandle));
}

SamplerHandle * OpenGLResourceContext::CreateSampler(ResourceCreationContext::SamplerCreateInfo const & ci)
{
    auto ret = (OpenGLSamplerHandle *)allocator.allocate(sizeof(OpenGLSamplerHandle));
    glCreateSamplers(1, &ret->nativeHandle);

    glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_WRAP_S, ToGLAddressMode(ci.addressModeU));
    glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_WRAP_T, ToGLAddressMode(ci.addressModeV));
    glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_MAG_FILTER, ToGLFilter(ci.magFilter));
    glSamplerParameteri(ret->nativeHandle, GL_TEXTURE_MIN_FILTER, ToGLFilter(ci.minFilter));
    return ret;
}

void OpenGLResourceContext::DestroySampler(SamplerHandle * handle)
{
    assert(handle != nullptr);
    auto handleGL = (OpenGLSamplerHandle *)handle;
    assert(handleGL->nativeHandle != 0);
    glDeleteSamplers(1, &handleGL->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLSamplerHandle));
}

DescriptorSetLayoutHandle * OpenGLResourceContext::CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo const & ci)
{
    auto mem = (void *)allocator.allocate(sizeof(OpenGLDescriptorSetLayoutHandle));
    auto ret = new (mem) OpenGLDescriptorSetLayoutHandle();
    ret->bindings = std::vector<ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding>(ci.bindingCount);
    memcpy(&(ret->bindings[0]),
           ci.pBinding,
           sizeof(ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding) * ci.bindingCount);
    return ret;
}

void OpenGLResourceContext::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle * handle)
{
    assert(handle != nullptr);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLDescriptorSetLayoutHandle));
}

VertexInputStateHandle *
OpenGLResourceContext::CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo & ci)
{
    auto ret = (OpenGLVertexInputStateHandle *)allocator.allocate(sizeof(OpenGLVertexInputStateHandle));
    glCreateVertexArrays(1, &ret->nativeHandle);
    for (size_t i = 0; i < ci.vertexAttributeDescriptions.size(); ++i) {
        auto & attributeDescription = ci.vertexAttributeDescriptions[i];
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
            logger->Errorf("Unknown vertex input type %d", ToUnderlyingType(attributeDescription.type));
        }

        glVertexArrayAttribFormat(ret->nativeHandle,
                                  attributeDescription.location,
                                  attributeDescription.size,
                                  type,
                                  attributeDescription.normalized,
                                  attributeDescription.offset);
        glVertexArrayAttribBinding(ret->nativeHandle, attributeDescription.location, attributeDescription.binding);
        glEnableVertexArrayAttrib(ret->nativeHandle, attributeDescription.location);
    }
    return ret;
}

void OpenGLResourceContext::DestroyVertexInputState(VertexInputStateHandle * handle)
{
    assert(handle != nullptr);
    auto nativeHandle = (OpenGLVertexInputStateHandle *)handle;
    assert(nativeHandle->nativeHandle != 0);
    glDeleteVertexArrays(1, &nativeHandle->nativeHandle);
    allocator.deallocate((uint8_t *)handle, sizeof(OpenGLVertexInputStateHandle));
}

DescriptorSet * OpenGLResourceContext::CreateDescriptorSet(DescriptorSetCreateInfo const & ci)
{
    auto mem = allocator.allocate(sizeof(OpenGLDescriptorSet));
    auto ret = new (mem) OpenGLDescriptorSet();
    ret->descriptors = std::vector<ResourceCreationContext::DescriptorSetCreateInfo::Descriptor>(ci.descriptorCount);
    memcpy(&(ret->descriptors[0]),
           ci.descriptors,
           ci.descriptorCount * sizeof(ResourceCreationContext::DescriptorSetCreateInfo::Descriptor));
    return ret;
}

void OpenGLResourceContext::DestroyDescriptorSet(DescriptorSet * set)
{
    assert(set != nullptr);
    allocator.deallocate((uint8_t *)set, sizeof(OpenGLDescriptorSet));
}

SemaphoreHandle * OpenGLResourceContext::CreateSemaphore()
{
    auto mem = allocator.allocate(sizeof(OpenGLSemaphoreHandle));
    auto ret = new (mem) OpenGLSemaphoreHandle;
    return ret;
}

void OpenGLResourceContext::DestroySemaphore(SemaphoreHandle * sem)
{
    auto nativeHandle = (OpenGLSemaphoreHandle *)sem;
    allocator.deallocate((uint8_t *)nativeHandle, sizeof(OpenGLSemaphoreHandle));
}

CommandBufferAllocator * OpenGLResourceContext::CreateCommandBufferAllocator()
{
    auto mem = allocator.allocate(sizeof(OpenGLCommandBufferAllocator));
    auto ret = new (mem) OpenGLCommandBufferAllocator();
    return ret;
}

void OpenGLResourceContext::DestroyCommandBufferAllocator(CommandBufferAllocator * alloc)
{
    assert(alloc != nullptr);
    allocator.deallocate((uint8_t *)alloc, sizeof(OpenGLCommandBufferAllocator));
}

FenceHandle * OpenGLResourceContext::CreateFence(bool startSignaled)
{
    auto mem = allocator.allocate(sizeof(OpenGLFenceHandle));
    auto ret = new (mem) OpenGLFenceHandle();
    if (startSignaled) {
        ret->sem.Signal();
    }
    return ret;
}

void OpenGLResourceContext::DestroyFence(FenceHandle * fence)
{
    assert(fence != nullptr);
    auto nativeHandle = (OpenGLFenceHandle *)fence;
    // TODO: Probably unsafe since sem needs to be destroyed
    allocator.deallocate((uint8_t *)fence, sizeof(OpenGLFenceHandle));
}

#endif
