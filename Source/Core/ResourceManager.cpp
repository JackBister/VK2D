#include "Core/ResourceManager.h"

ResourceManager::ResourceManager(Queue<RenderCommand>::Writer&& writer, Allocator & a) : allocator(a), renderQueue(std::move(writer))
{
}

void ResourceManager::PushRenderCommand(const RenderCommand& c)
{
	renderQueue.Push(c);
}
