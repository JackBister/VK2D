#pragma once

#include "CommandBuffer.h"
#include "RenderResources.h"

struct CommandBufferAllocator {
    struct CommandBufferCreateInfo {
        CommandBufferLevel level;
    };
    virtual CommandBuffer * CreateBuffer(CommandBufferCreateInfo const & pCreateInfo) = 0;
    virtual void DestroyContext(CommandBuffer *) = 0;
    virtual void Reset() = 0;
};