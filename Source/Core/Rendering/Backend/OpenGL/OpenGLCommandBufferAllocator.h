#ifdef USE_OGL_RENDERER

#include "Core/Rendering/Backend/Abstract/CommandBufferAllocator.h"

struct OpenGLCommandBufferAllocator : CommandBufferAllocator
{
	CommandBuffer * CreateBuffer(CommandBufferCreateInfo const& createInfo) final override;

	void DestroyContext(CommandBuffer *) final override;

	void Reset() final override;
};

#endif
