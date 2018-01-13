#pragma once
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Resource.h"

struct ImageCreateInfo;
class ResourceManager;

class Image : public Resource
{
public:
	friend class Renderer;

	~Image();
	Image() = delete;
	Image(ResourceManager *, std::string const&);
	Image(ImageCreateInfo const&);
	Image(ResourceManager *, std::string const&, std::vector<uint8_t> const&);

	std::vector<uint8_t> const& GetData() const noexcept;
	uint32_t GetHeight() const noexcept;
	ImageHandle * GetImageHandle();
	uint32_t GetWidth() const noexcept;

private:
	std::vector<uint8_t> data;

	uint32_t width, height;
	ImageHandle * img;
	ResourceManager * resMan;
};

struct ImageCreateInfo
{
	std::string name;
	int height, width;
	ResourceManager * resMan;
	//The image is either created with CPU data or an existing rendererdata
	std::variant<std::vector<uint8_t>, ImageHandle *> data;
};
