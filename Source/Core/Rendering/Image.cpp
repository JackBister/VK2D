#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/ResourceManager.h"

Image::~Image()
{
	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		ctx.DestroyImage(img);
	});
}

Image::Image(ResourceManager * resMan, std::string const& name) : resMan(resMan)
{
	this->name = name;
	this->data = std::vector<uint8_t>(128 * 128 * 4, 0xFF);

	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			128, 128, 1,
			1,
			IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
		};
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data);
		this->img = tmp;
	});
}

Image::Image(ImageCreateInfo const& info) : resMan(info.resMan)
{
	this->name = info.name;
	this->width = info.width;
	this->height = info.height;
	if (info.data.index() == 0) {
		this->data = std::get<std::vector<uint8_t>>(info.data);
		info.resMan->CreateResources([this](ResourceCreationContext& ctx) {
			ResourceCreationContext::ImageCreateInfo ic = {
				Format::RGBA8,
				ImageHandle::Type::TYPE_2D,
				this->width, this->height, 1,
				1,
				IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
			};
			auto tmp = ctx.CreateImage(ic);
			ctx.ImageData(tmp, this->data);
			this->img = tmp;
		});
	} else {
		this->img = std::get<ImageHandle *>(info.data);
	}
}

Image::Image(ResourceManager * resMan, std::string const& name, std::vector<uint8_t> const& input) : resMan(resMan)
{
	this->name = name;
	int n;
	uint8_t const * imageData = stbi_load_from_memory((stbi_uc const *)&input[0], (int)input.size(), (int *)&width, (int *)&height, (int *)&n, 0);
	data = std::vector<uint8_t>(width * height * n);
	memcpy(&data[0], imageData, width * height * n);
	Format format = Format::RGBA8;
	//TODO: pads to RGBA8 in a terrible way
	{
		auto idx = 1;
		auto pad = data.size() % 4;
		for (auto it = data.begin(); it != data.end(); ++it) {
			if (idx % 4 == pad) {
				for (auto i = 0; i < 4 - pad; ++i) {
					it = data.insert(++it, 0xFF);
					idx++;
				}
			} else {
				idx++;
			}
		}
	}
	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			this->width, this->height, 1,
			1,
			IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
		};
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data);
		this->img = tmp;
	});
}

std::vector<uint8_t> const& Image::GetData() const noexcept
{
	return data;
}

uint32_t Image::GetHeight() const noexcept
{
	return height;
}

ImageHandle * Image::GetImageHandle()
{
	return img;
}

uint32_t Image::GetWidth() const noexcept
{
	return width;
}
