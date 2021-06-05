#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Logging/Logger.h"

class RenderSystem;
class ResourceCreationContext;

namespace ResourceManager
{
extern Logger logger;
extern std::unordered_map<std::string, void *> resources;

void CreateResources(std::function<void(ResourceCreationContext &)> && fun);
void DestroyResources(std::function<void(ResourceCreationContext &)> && fun);
void Init(RenderSystem * renderSystem);

template <typename T>
void AddResource(std::string const & name, T * resource)
{
    auto newName = std::filesystem::path(name).make_preferred().string();
    logger.Info("Adding resource '{}' = {}", newName, resource);
    resources.insert_or_assign(newName, resource);
}

template <typename T>
T * GetResource(std::string const & name)
{
    auto newName = std::filesystem::path(name).make_preferred().string();
    if (resources.find(newName) == resources.end()) {
        logger.Info("Resource '{}' not found", newName);
        return nullptr;
    }
    return (T *)resources.at(newName);
}
}
