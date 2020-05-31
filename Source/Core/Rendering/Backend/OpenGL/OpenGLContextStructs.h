#pragma once
#ifdef USE_OGL_RENDERER
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLResourceContext.h"

#include "Core/Semaphore.h"

struct OpenGLImageViewHandle;

struct OpenGLBufferHandle : BufferHandle {
    GLuint nativeHandle = 0;
    size_t offset = 0;
    GLenum elementType = 0;
    uint32_t usage = 0;
    uint32_t memoryProperties = 0;
};

struct OpenGLDescriptorSet : DescriptorSet {
    std::vector<ResourceCreationContext::DescriptorSetCreateInfo::Descriptor> descriptors;
};

struct OpenGLDescriptorSetLayoutHandle : DescriptorSetLayoutHandle {
    std::vector<ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding> bindings;
};

struct OpenGLFenceHandle : FenceHandle {
    bool Wait(uint64_t timeOut) final override;

    Semaphore sem;
};

struct OpenGLFramebufferHandle : FramebufferHandle {
    GLuint nativeHandle = 0;

    std::vector<OpenGLImageViewHandle *> attachments;
    Format format;
    uint32_t width, height, layers;
};

struct OpenGLImageHandle : ImageHandle {
    GLuint nativeHandle = 0;

    uint32_t mipLevels;
};

struct OpenGLImageViewHandle : ImageViewHandle {
    ImageHandle * image;
    ImageSubresourceRange subresourceRange;
};

struct OpenGLRenderPassHandle : RenderPassHandle {
    ResourceCreationContext::RenderPassCreateInfo createInfo;
};

struct OpenGLPipelineLayoutHandle : PipelineLayoutHandle {
    std::vector<DescriptorSetLayoutHandle *> descriptorLayouts;
};

struct OpenGLPipelineHandle : PipelineHandle {
    GLuint nativeHandle;
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo rasterizationState;
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly;
};

struct OpenGLSamplerHandle : SamplerHandle {
    GLuint nativeHandle;
};

struct OpenGLSemaphoreHandle : SemaphoreHandle {
    Semaphore sem;
};

struct OpenGLShaderModuleHandle : ShaderModuleHandle {
    GLuint nativeHandle;
};

struct OpenGLVertexInputStateHandle : VertexInputStateHandle {
    GLuint nativeHandle;
};

#endif