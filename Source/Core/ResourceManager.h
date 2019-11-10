#pragma once
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "Core/Allocator.h"
#include "Core/Queue.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Resource.h"

class ResourceManager
{
  public:
    ResourceManager(Renderer * renderer, Allocator & a = Allocator::default_allocator);

    template <typename T>
    void AddResource(std::string const & name, T * res);

    template <typename T>
    void AddResourceRefCounted(std::string const & name, std::weak_ptr<T> const res);

    template <typename T>
    T * GetResource(std::string const & name);

    template <typename T>
    std::shared_ptr<T> LoadResource(std::string const & fileName);

    template <typename T, typename... ArgTypes>
    std::shared_ptr<T> LoadResourceOrConstruct(std::string const & fileName, ArgTypes &&... args);

    void CreateResources(std::function<void(ResourceCreationContext &)> fun);

  private:
    std::unordered_map<std::string, void *> nonRCCache;
    std::unordered_map<std::string, std::weak_ptr<void>> rcCache;
    Allocator & allocator;
    Renderer * renderer;
};

template <typename T>
void ResourceManager::AddResource(std::string const & name, T * res)
{
    nonRCCache[name] = (void *)res;
}

template <typename T>
void ResourceManager::AddResourceRefCounted(std::string const & name, std::weak_ptr<T> const res)
{
    // TODO: Check if exists? Pointless?
    rcCache[name] = res;
}

template <typename T>
T * ResourceManager::GetResource(std::string const & name)
{
    if (nonRCCache.find(name) != nonRCCache.end() && nonRCCache[name] != nullptr) {
        return (T *)nonRCCache[name];
    }
    return nullptr;
}

template <typename T>
std::shared_ptr<T> ResourceManager::LoadResource(std::string const & fileName)
{
    if (rcCache.find(fileName) != rcCache.end() && !rcCache[fileName].expired()) {
        auto ret = std::static_pointer_cast<T>(rcCache[fileName].lock());
        if (ret) {
            return ret;
        }
    }
    std::filesystem::path filePath(fileName);
    auto status = std::filesystem::status(filePath);
    Resource * mem = (Resource *)allocator.Allocate(sizeof(T));
    if (status.type() != std::filesystem::file_type::regular) {
        auto ret = std::shared_ptr<T>(new (mem) T(this, fileName));
        rcCache[fileName] = std::weak_ptr<void>(ret);
        return ret;
    } else {
        // For some awful reason this wont work for binary files like PNG. Otherwise I think it's a
        // prettier solution.
#if 0
		std::filesystem::ifstream is(fileName, std::ios::in | std::ios::binary);
		auto ret = std::shared_ptr<T>(new (mem) T(fileName, is));
#endif
        FILE * f = fopen(fileName.c_str(), "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            size_t length = ftell(f);
            fseek(f, 0, SEEK_SET);
            std::vector<uint8_t> buf(length + 1);
            fread(&buf[0], 1, length, f);
            auto ret = std::shared_ptr<T>(new (mem) T(this, fileName, buf));
            rcCache[fileName] = std::weak_ptr<void>(ret);
            fclose(f);
            return ret;
        } else {
            auto ret = std::shared_ptr<T>(new (mem) T(this, fileName));
            rcCache[fileName] = std::weak_ptr<void>(ret);
            return ret;
        }
    }
}

template <typename T, typename... ArgTypes>
std::shared_ptr<T> ResourceManager::LoadResourceOrConstruct(std::string const & fileName,
                                                            ArgTypes &&... args)
{
    if (rcCache.find(fileName) != rcCache.end() && !rcCache[fileName].expired()) {
        auto ret = std::static_pointer_cast<T>(rcCache[fileName].lock());
        if (ret) {
            return ret;
        }
    }

    Resource * mem = (Resource *)allocator.Allocate(sizeof(T));
    auto ret = std::shared_ptr<T>(new (mem) T(this, fileName, args...));
    rcCache[fileName] = std::weak_ptr<void>(ret);
    return ret;
}
