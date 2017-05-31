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
#include "Core/Rendering/RendererData.h"
#include "Core/Resource.h"

struct ImageCreateInfo;
struct ResourceManager;

struct Image : Resource
{
	friend struct Renderer;

	Image() = delete;
	Image(ResourceManager *, const std::string&) noexcept;
	Image(const ImageCreateInfo&) noexcept;
	Image(ResourceManager *, const std::string&, const std::vector<uint8_t>&) noexcept;

	const std::vector<uint8_t>& GetData() const noexcept;
	uint32_t GetHeight() const noexcept;
	ImageHandle * GetImageHandle();
	uint32_t GetWidth() const noexcept;

private:
	std::vector<uint8_t> data;

	uint32_t width, height;
	ImageHandle * img;
};

struct ImageCreateInfo
{
	std::string name;
	int height, width;
	ResourceManager * resMan;
	//The image is either created with CPU data or an existing rendererdata
	std::variant<std::vector<uint8_t>, ImageHandle *> data;
};
