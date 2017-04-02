#include "Core/ResourceManager.h"

ResourceManager::ResourceManager(Queue<RenderCommand>::Writer&& writer, Allocator & a) : bufferPool(this, 4096 * 1024), allocator(a), renderQueue(std::move(writer))
{
}

void ResourceManager::PushRenderCommand(const RenderCommand& c)
{
	renderQueue.Push(c);
}
