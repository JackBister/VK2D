#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Core/Logging/Logger.h"

class Renderer;
class ResourceCreationContext;

namespace ResourceManager
{
extern std::unique_ptr<Logger> logger;
extern std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> fun);
void Init(Renderer * renderer);

template <typename T>
void AddResource(std::string const & name, T * resource)
{
    logger->Infof("Adding resource '%s' = %p", name.c_str(), resource);
    resources.insert_or_assign(name, resource);
}

template <typename T>
T * GetResource(std::string const & name)
{
    if (resources.find(name) == resources.end()) {
        logger->Infof("Resource '%s' not found", name.c_str());
        return nullptr;
    }
    return (T *)resources.at(name);
}
}
