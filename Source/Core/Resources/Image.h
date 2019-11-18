﻿#pragma once
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#if HOT_RELOAD_RESOURCES
#include <functional>
#include <unordered_map>
#endif

#include "Core/Rendering/Backend/Abstract/RenderResources.h"

class Image
{
public:
    ~Image();

    static Image * FromFile(std::string const &, bool forceReload = false);

    uint32_t GetHeight() const;
    uint32_t GetWidth() const;

    ImageViewHandle * GetDefaultView();
    ImageHandle * GetImage() const;

#if HOT_RELOAD_RESOURCES
    int SubscribeToChanges(std::function<void(Image *)> cb);
    void Unsubscribe(int subscriptionId);
#endif

private:
    Image(std::string const & fileName, uint32_t width, uint32_t height, ImageHandle * img,
          ImageViewHandle * defaultView);

    std::string fileName;

    uint32_t width, height;
    ImageHandle * img;

    ImageViewHandle * defaultView;

#if HOT_RELOAD_RESOURCES
    int hotReloadSubscriberId = 0;
    std::unordered_map<int, std::function<void(Image *)>> hotReloadCallbacks;
#endif
};