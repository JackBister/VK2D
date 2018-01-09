#ifdef USE_OGL_RENDERER

#include "Core/Rendering/Context/RenderContext.h"

struct OpenGLCommandBufferAllocator : CommandContextAllocator
{
	CommandBuffer * CreateContext(CommandBufferCreateInfo const& createInfo) final override;

	void DestroyContext(CommandBuffer *) final override;

	void Reset() final override;
};

#endif
