#include "Core/ResourceManager.h"

#include "Core/Rendering/Renderer.h"

ResourceManager::ResourceManager(Renderer * renderer,/* Queue<RenderCommand>::Writer&& writer, */Allocator & a) : /*bufferPool(this, 4096 * 1024),*/ allocator(a), renderer(renderer)//, renderQueue(std::move(writer))
{
}

void ResourceManager::CreateResources(std::function<void(ResourceCreationContext&)> fun)
{
	renderer->CreateResources(fun);
}

/*
void ResourceManager::PushRenderCommand(RenderCommand&& c)
{
	renderQueue.Push(std::move(c));
}
*/
