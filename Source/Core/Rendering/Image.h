#pragma once
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

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

private:
	Image(std::string const & fileName, uint32_t width, uint32_t height, ImageHandle * img);

    std::string fileName;

    uint32_t width, height;
    ImageHandle * img;

    ImageViewHandle * defaultView = nullptr;
};
