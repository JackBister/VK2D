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
};

struct OpenGLImageViewHandle : ImageViewHandle {
    ImageHandle * image;
    ImageSubresourceRange subresourceRange;
};

struct OpenGLRenderPassHandle : RenderPassHandle {
};

struct OpenGLPipelineLayoutHandle : PipelineLayoutHandle {
    std::vector<DescriptorSetLayoutHandle *> descriptorLayouts;
};

struct OpenGLPipelineHandle : PipelineHandle {
    GLuint nativeHandle;
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo rasterizationState;
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