#include "Core/ResourceManager.h"

#include "Core/Rendering/Backend/Renderer.h"

ResourceManager::ResourceManager(Renderer * renderer, Allocator & a)
    : allocator(a), renderer(renderer)
{
}

void ResourceManager::CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderer->CreateResources(fun);
}
