#include "ResourceManager.h"

#include <optick/optick.h>

#include "Core/Rendering/RenderSystem.h"

namespace ResourceManager
{
Logger logger = Logger::Create("ResourceManager");
RenderSystem * renderSystem;
std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> && fun)
{
    OPTICK_EVENT();
    renderSystem->CreateResources(std::move(fun));
}

void DestroyResources(std::function<void(ResourceCreationContext &)> && fun)
{
    OPTICK_EVENT();
    renderSystem->DestroyResources(std::move(fun));
}

void Init(RenderSystem * inRenderSystem)
{
    renderSystem = inRenderSystem;
}
}
