#pragma once
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
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

	std::vector<uint8_t> const& GetData() const;
	uint32_t GetHeight() const;
	ImageHandle * GetImage();
	ImageViewHandle * GetImageView();
	SamplerHandle * GetSampler();
	uint32_t GetWidth() const;

private:
	std::vector<uint8_t> data;

	uint32_t width, height;
	ImageHandle * img;
	ImageViewHandle * view;
	SamplerHandle * sampler;
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
