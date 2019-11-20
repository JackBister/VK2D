#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Core/Logging/Logger.h"

class RenderSystem;
class ResourceCreationContext;

namespace ResourceManager
{
extern std::unique_ptr<Logger> logger;
extern std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> fun);
void DestroyResources(std::function<void(ResourceCreationContext &)> fun);
void Init(RenderSystem * renderSystem);

template <typename T>
void AddResource(std::string const & name, T * resource)
{
    auto newName = std::filesystem::path(name).make_preferred().string();
    logger->Infof("Adding resource '%s' = %p", newName.c_str(), resource);
    resources.insert_or_assign(newName, resource);
}

template <typename T>
T * GetResource(std::string const & name)
{
    auto newName = std::filesystem::path(name).make_preferred().string();
    if (resources.find(newName) == resources.end()) {
        logger->Infof("Resource '%s' not found", newName.c_str());
        return nullptr;
    }
    return (T *)resources.at(newName);
}
}
