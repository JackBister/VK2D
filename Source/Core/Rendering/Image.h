#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include <experimental/variant.hpp>

#include "Core/Resource.h"
#include "Rendering/render.h"

struct ImageCreateInfo;

struct Image : Resource
{
	enum class Format
	{
		DEPTH,
		DEPTH_STENCIL,
		R8,
		RG8,
		RGB8,
		RGBA8
	};

	Image() = delete;
	Image(ResourceManager *, const std::string&) noexcept;
	Image(const ImageCreateInfo&) noexcept;
	Image(ResourceManager *, const std::string&, const std::vector<char>&) noexcept;

	const std::vector<uint8_t>& GetData() const noexcept;
	Format GetFormat() const noexcept;
	int GetHeight() const noexcept;
	int GetWidth() const noexcept;
	void * GetRendererData() const noexcept;
	void SetRendererData(void *) noexcept;

private:
	Format format;
	int height, width;
	std::vector<uint8_t> data;
	
	void * rendererData;
};

struct ImageCreateInfo
{
	std::string name;
	Image::Format format;
	int height, width;
	//The image is either created with CPU data or an existing rendererdata
	std::experimental::variant<std::vector<uint8_t>, void *> data;
};

RESOURCE_HEADER(Image)
