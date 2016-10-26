#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "Core/Resource.h"

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
	Image(const std::string&) noexcept;
	Image(const std::string&, const std::vector<char>&) noexcept;

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

RESOURCE_HEADER(Image)
