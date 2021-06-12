#include "BufferAllocator.h"

#include <cassert>

#include <optick/optick.h>

#include "Logging/Logger.h"
#include "RenderSystem.h"
#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Abstract/ResourceCreationContext.h"
#include "Util/Semaphore.h"

auto const logger = Logger::Create("BufferAllocator");

BufferAllocator * BufferAllocator::instance = nullptr;
size_t const MIN_BUFFER_SIZE = 2 * 1024 * 1024;

BufferAllocator * BufferAllocator::GetInstance()
{
    return BufferAllocator::instance;
}

BufferAllocator::BufferAllocator(RenderSystem * renderSystem) : renderSystem(renderSystem)
{
    BufferAllocator::instance = this;
}

BufferSlice BufferAllocator::AllocateBuffer(size_t size, uint32_t bufferUsage, uint32_t memoryProperties)
{
    OPTICK_EVENT();
    assert(size > 0);
    logger.Info(
        "Looking for buffer with size={}, bufferUsage={}, memoryProperties={}", size, bufferUsage, memoryProperties);
    // TODO: There is currently nothing that removes FreeListNodes from the list when their size hits 0
    for (auto & freeListNode : freeList) {
        if (freeListNode.size >= size && buffers[freeListNode.bufferIdx].bufferUsage == bufferUsage &&
            buffers[freeListNode.bufferIdx].memoryProperties == memoryProperties) {
            logger.Info("Found FreeListNode with bufferIdx={}, buffer={}, offset={}, size={}",
                        freeListNode.bufferIdx,
                        buffers[freeListNode.bufferIdx].buffer,
                        freeListNode.offset,
                        freeListNode.size);
            freeListNode.offset += size;
            freeListNode.size -= size;
            logger.Info("FreeListNode after allocation: offset={}, size={}", freeListNode.offset, freeListNode.size);
            return BufferSlice(buffers[freeListNode.bufferIdx].buffer, freeListNode.offset, size);
        }
    }
    logger.Info("No matching FreeListNode found for allocation request with size={}, bufferUsage={}, "
                "memoryProperties={}, will allocate a new buffer.",
                size,
                bufferUsage,
                memoryProperties);
    // If we end up here, there was no buffer large enough in the free list, so we allocate a new one
    BufferHandle * ret;
    Semaphore sem;
    this->renderSystem->CreateResources(
        [this, &ret, &sem, size, bufferUsage, memoryProperties](ResourceCreationContext & ctx) {
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = (MemoryPropertyFlagBits)memoryProperties;
            // We always round up to a multiplier of MIN_BUFFER_SIZE
            size_t multiplier = (size + MIN_BUFFER_SIZE - 1) / MIN_BUFFER_SIZE;
            ci.size = multiplier * MIN_BUFFER_SIZE;
            ci.usage = bufferUsage;
            auto buf = ctx.CreateBuffer(ci);
            logger.Info("Allocated a new buffer={} with size={}", buf, ci.size);
            BufferMetadata metadata{buf, bufferUsage, memoryProperties};
            this->buffers.push_back(metadata);
            if (size != ci.size) {
                FreeListNode freeListNode;
                freeListNode.bufferIdx = this->buffers.size() - 1;
                freeListNode.offset = size;
                freeListNode.size = ci.size - size;
                logger.Info("Inserting new FreeListNode with bufferIdx={}, buffer={}, offset={}, size={}",
                            freeListNode.bufferIdx,
                            buffers[freeListNode.bufferIdx].buffer,
                            freeListNode.offset,
                            freeListNode.size);
                this->freeList.push_back(freeListNode);
            }
            ret = buf;
            sem.Signal();
        });
    sem.Wait();
    return BufferSlice(ret, 0, size);
}

void BufferAllocator::FreeBuffer(BufferSlice slice)
{
    OPTICK_EVENT();
    for (size_t i = 0; i < buffers.size(); ++i) {
        if (buffers[i].buffer == slice.GetBuffer()) {
            FreeListNode newNode;
            newNode.bufferIdx = i;
            newNode.offset = slice.GetOffset();
            newNode.size = slice.GetSize();
            freeList.push_back(newNode);
            return;
        }
    }
    logger.Warn("Failed to free buffer={}. Was it not allocated by the BufferAllocator?", slice.GetBuffer());
}
