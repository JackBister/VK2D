#ifdef USE_OGL_RENDERER

#include "Core/Rendering/Context/RenderContext.h"

struct OpenGLCommandBufferAllocator : CommandBufferAllocator
{
	CommandBuffer * CreateBuffer(CommandBufferCreateInfo const& createInfo) final override;

	void DestroyContext(CommandBuffer *) final override;

	void Reset() final override;
};

#endif
