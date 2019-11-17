#include "Core/ResourceManager.h"

#include "Core/Rendering/Backend/Renderer.h"

namespace ResourceManager
{
std::unique_ptr<Logger> logger = Logger::Create("ResourceManager");
Renderer * renderer;
std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderer->CreateResources(fun);
}

void Init(Renderer * inRenderer)
{
    renderer = inRenderer;
}
}
