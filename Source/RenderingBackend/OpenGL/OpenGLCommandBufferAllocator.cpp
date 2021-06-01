#ifdef USE_OGL_RENDERER
#include "OpenGLCommandBufferAllocator.h"

#include <cassert>

#include "OpenGLCommandBuffer.h"

CommandBuffer * OpenGLCommandBufferAllocator::CreateBuffer(CommandBufferCreateInfo const & createInfo)
{
    return new OpenGLCommandBuffer(new std::allocator<uint8_t>());
}

void OpenGLCommandBufferAllocator::DestroyContext(CommandBuffer * ctx)
{
    assert(ctx != nullptr);
    delete ctx;
}

void OpenGLCommandBufferAllocator::Reset() {}

#endif