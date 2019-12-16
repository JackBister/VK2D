#pragma once

#include <cstdint>
#include <future>
#include <vector>

#include "BufferSlice.h"

class BufferHandle;
class RenderSystem;

class BufferAllocator
{
public:
    static BufferAllocator * GetInstance();

    BufferAllocator(RenderSystem * renderSystem);

    BufferSlice AllocateBuffer(size_t size, uint32_t bufferUsage, uint32_t memoryProperties);
    void FreeBuffer(BufferSlice slice);

private:
    static BufferAllocator * instance;

    struct BufferMetadata {
        BufferHandle * buffer;

        uint32_t bufferUsage;
        uint32_t memoryProperties;
    };

    struct FreeListNode {
        size_t bufferIdx;
        size_t offset;
        size_t size;
    };

    std::vector<BufferMetadata> buffers;
    // TODO: Maybe this should actually be a list...
    std::vector<FreeListNode> freeList;

    RenderSystem * renderSystem;
};
