#pragma once
#include <cstddef>
#include <string>
#include <vector>

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif

#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Resource.h"

struct ImageCreateInfo;
class ResourceManager;

class Image : public Resource
{
public:
	friend class Renderer;

	Image() = delete;
	Image(ResourceManager *, std::string const&) noexcept;
	Image(ImageCreateInfo const&) noexcept;
	Image(ResourceManager *, std::string const&, std::vector<uint8_t> const&) noexcept;

	std::vector<uint8_t> const& get_data() const noexcept;
	uint32_t get_height() const noexcept;
	ImageHandle * get_image_handle();
	uint32_t get_width() const noexcept;

private:
	std::vector<uint8_t> data_;

	uint32_t width_, height_;
	ImageHandle * img_;
};

struct ImageCreateInfo
{
	std::string name;
	int height, width;
	ResourceManager * resMan;
	//The image is either created with CPU data or an existing rendererdata
	std::variant<std::vector<uint8_t>, ImageHandle *> data;
};
