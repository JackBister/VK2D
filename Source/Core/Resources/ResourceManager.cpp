#include "ResourceManager.h"

#include "Core/Rendering/RenderSystem.h"

namespace ResourceManager
{
std::unique_ptr<Logger> logger = Logger::Create("ResourceManager");
RenderSystem * renderSystem;
std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderSystem->CreateResources(fun);
}

void DestroyResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderSystem->DestroyResources(fun);
}

void Init(RenderSystem * inRenderSystem)
{
    renderSystem = inRenderSystem;
}
}
