#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "BufferSlice.h"

class BufferHandle;
class Renderer;

struct MappedBuffer {
    void * basePtr;
    std::vector<void *> allMapped;
};

class BufferAllocator
{
public:
    static BufferAllocator * GetInstance();

    BufferAllocator(Renderer * renderer);

    BufferSlice AllocateBuffer(size_t size, uint32_t bufferUsage, uint32_t memoryProperties);
    void FreeBuffer(BufferSlice slice);

    void * MapBuffer(BufferSlice slice);
    void UnmapBuffer(void *);

private:
    static BufferAllocator * instance;

    struct BufferMetadata {
        BufferHandle * buffer;
        size_t allocatedSize;

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

    std::unordered_map<void *, BufferHandle *> isMappedFrom;
    std::unordered_map<BufferHandle *, MappedBuffer> mappedBuffers;

    Renderer * renderer;
};
