#include "BufferAllocator.h"

#include <algorithm>
#include <cassert>

#include <ThirdParty/optick/src/optick.h>

#include "Logging/Logger.h"
#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Abstract/ResourceCreationContext.h"
#include "RenderingBackend/Renderer.h"
#include "Util/Semaphore.h"

auto const logger = Logger::Create("BufferAllocator");

BufferAllocator * BufferAllocator::instance = nullptr;
size_t const MIN_BUFFER_SIZE = 2 * 1024 * 1024;

BufferAllocator * BufferAllocator::GetInstance()
{
    return BufferAllocator::instance;
}

BufferAllocator::BufferAllocator(Renderer * renderer) : renderer(renderer)
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
    // TODO: This is completely thread-unsafe...
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
    this->renderer->CreateResources(
        [this, &ret, &sem, size, bufferUsage, memoryProperties](ResourceCreationContext & ctx) {
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = (MemoryPropertyFlagBits)memoryProperties;
            // We always round up to a multiplier of MIN_BUFFER_SIZE
            size_t multiplier = (size + MIN_BUFFER_SIZE - 1) / MIN_BUFFER_SIZE;
            ci.size = multiplier * MIN_BUFFER_SIZE;
            ci.usage = bufferUsage;
            auto buf = ctx.CreateBuffer(ci);
            logger.Info("Allocated a new buffer={} with size={}", buf, ci.size);
            BufferMetadata metadata{buf, ci.size, bufferUsage, memoryProperties};
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

void * BufferAllocator::MapBuffer(BufferSlice slice)
{
    OPTICK_EVENT();
    logger.Info("Mapping slice={}", slice);
    auto alreadyMapped = mappedBuffers.find(slice.GetBuffer());
    if (alreadyMapped != mappedBuffers.end()) {
        logger.Info("buffer={} is already mapped, will do mapping using basePtr", slice.GetBuffer());
        void * finalPtr = ((uint8_t *)alreadyMapped->second.basePtr) + slice.GetOffset();
        alreadyMapped->second.allMapped.push_back(finalPtr);
        return finalPtr;
    } else {
        logger.Info("buffer={} is not already mapped, will do mapping using CreateResources", slice.GetBuffer());
        size_t fullSize = 0;
        for (auto const & b : buffers) {
            if (b.buffer == slice.GetBuffer()) {
                fullSize = b.allocatedSize;
            }
        }
        if (fullSize == 0) {
            logger.Error("fullSize was 0 when mapping buffer={}. Was this buffer not allocated by BufferAllocator? "
                         "Will return null.",
                         slice.GetBuffer());
            return nullptr;
        }
        uint8_t * basePtr = nullptr;
        Semaphore sem;
        renderer->CreateResources([&basePtr, fullSize, &sem, &slice](ResourceCreationContext & ctx) {
            basePtr = ctx.MapBuffer(slice.GetBuffer(), 0, fullSize);
            sem.Signal();
        });
        sem.Wait();
        if (!basePtr) {
            logger.Error("basePtr was null after mapping using CreateResources when mapping buffer={}",
                         slice.GetBuffer());
            return nullptr;
        }
        void * finalPtr = basePtr + slice.GetOffset();
        logger.Info("Got basePtr={}, finalPtr={} when mapping slice={}", basePtr, finalPtr, slice);

        mappedBuffers.insert({slice.GetBuffer(), {.basePtr = basePtr, .allMapped = {finalPtr}}});
        return finalPtr;
    }
}
void BufferAllocator::UnmapBuffer(void * ptr)
{
    OPTICK_EVENT();
    logger.Info("Unmapping ptr={}", ptr);
    auto mappedFrom = isMappedFrom.find(ptr);
    if (mappedFrom == isMappedFrom.end()) {
        logger.Warn("Attempted to unmap ptr={} but did not find which buffer it was mapped from. Perhaps it has "
                    "already been unmapped?",
                    ptr);
        return;
    }
    auto allMapped = mappedBuffers.find(mappedFrom->second);
    if (allMapped == mappedBuffers.end()) {
        isMappedFrom.erase(mappedFrom);
        logger.Warn("Found bufferHandle={} for ptr={} when unmapping but did not find it in the mappedBuffers map",
                    mappedFrom->second,
                    mappedFrom->first);
        return;
    }

    BufferHandle * bufferHandle = mappedFrom->second;
    logger.Info("Erasing ptr={} from isMappedFrom and allMapped. isMappedFromSizeBefore={}, allMappedSizeBefore={}",
                ptr,
                isMappedFrom.size(),
                allMapped->second.allMapped.size());
    isMappedFrom.erase(mappedFrom);
    std::erase(allMapped->second.allMapped, ptr);
    logger.Info("After erasing ptr={} from isMappedFrom and allMapped. isMappedFromSizeAfter={}, allMappedSizeAfter={}",
                ptr,
                isMappedFrom.size(),
                allMapped->second.allMapped.size());
    if (allMapped->second.allMapped.size() == 0) {
        logger.Info("allMapped size was 0 after erasing ptr={}. Will unmap bufferHandle={} completely.",
                    ptr,
                    mappedFrom->second);
        mappedBuffers.erase(allMapped);
        Semaphore sem;
        renderer->CreateResources([bufferHandle, &sem](ResourceCreationContext & ctx) {
            ctx.UnmapBuffer(bufferHandle);
            sem.Signal();
        });
        sem.Wait();
    }
}
