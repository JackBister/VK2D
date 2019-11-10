#pragma once

#include "Core/Rendering/Backend/Abstract/CommandBuffer.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"

struct CommandBufferAllocator
{
	struct CommandBufferCreateInfo
	{
		CommandBufferLevel level;
	};
	virtual CommandBuffer * CreateBuffer(CommandBufferCreateInfo const& pCreateInfo) = 0;
	virtual void DestroyContext(CommandBuffer *) = 0;
	virtual void Reset() = 0;
};