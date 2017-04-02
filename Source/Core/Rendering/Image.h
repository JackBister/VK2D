#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include <experimental/variant.hpp>

#include "Core/Rendering/RendererData.h"
#include "Core/Resource.h"

struct ImageCreateInfo;
struct ResourceManager;

struct Image : Resource
{
	friend class Renderer;

	enum class Format
	{
		R8,
		RG8,
		RGB8,
		RGBA8
	};

	enum class MinFilter
	{
		LINEAR,
		LINEAR_MIPMAP_LINEAR,
		LINEAR_MIPMAP_NEAREST,
		NEAREST,
		NEAREST_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
	};

	enum class MagFilter
	{
		LINEAR,
		NEAREST
	};

	enum class WrapMode
	{
		CLAMP_TO_BORDER,
		CLAMP_TO_EDGE,
		MIRROR_CLAMP_TO_EDGE,
		MIRRORED_REPEAT,
		REPEAT
	};
	
	struct ImageParameters
	{
		WrapMode sWrapMode = WrapMode::REPEAT;
		WrapMode tWrapMode = WrapMode::REPEAT;
		WrapMode rWrapMode = WrapMode::REPEAT;

		MinFilter minFilter = MinFilter::NEAREST_MIPMAP_LINEAR;
		MagFilter magFilter = MagFilter::LINEAR;
	};

	Image() = delete;
	Image(ResourceManager *, const std::string&) noexcept;
	Image(const ImageCreateInfo&) noexcept;
	Image(ResourceManager *, const std::string&, const std::vector<uint8_t>&) noexcept;

	const std::vector<uint8_t>& GetData() const noexcept;
	Format GetFormat() const noexcept;
	int GetHeight() const noexcept;
	const ImageParameters& GetParameters() const noexcept;
	int GetWidth() const noexcept;
	const ImageRendererData& GetRendererData() const noexcept;
	void SetRendererData(const ImageRendererData&) noexcept;

private:
	Format format;
	int height, width;
	std::vector<uint8_t> data;

	ImageParameters params;

	ImageRendererData rendererData;
};

struct ImageCreateInfo
{
	std::string name;
	Image::Format format;
	int height, width;
	Image::ImageParameters params;
	ResourceManager * resMan;
	//The image is either created with CPU data or an existing rendererdata
	std::experimental::variant<std::vector<uint8_t>, ImageRendererData> data;
};
