#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/ResourceManager.h"

Image::~Image()
{
	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		ctx.DestroyImage(img);
		ctx.DestroyImageView(view);
		ctx.DestroySampler(sampler);
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

		ResourceCreationContext::ImageViewCreateInfo ivc = {};
		ivc.components.r = ComponentSwizzle::R;
		ivc.components.g = ComponentSwizzle::G;
		ivc.components.b = ComponentSwizzle::B;
		ivc.components.a = ComponentSwizzle::A;
		ivc.format = Format::RGBA8;
		ivc.image = this->img;
		ivc.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
		ivc.subresourceRange.baseArrayLayer = 0;
		ivc.subresourceRange.baseMipLevel = 0;
		ivc.subresourceRange.layerCount = 1;
		ivc.subresourceRange.levelCount = 1;
		ivc.viewType = ImageViewHandle::Type::TYPE_2D;
		this->view = ctx.CreateImageView(ivc);

		ResourceCreationContext::SamplerCreateInfo sc = {};
		sc.addressModeU = AddressMode::REPEAT;
		sc.addressModeV = AddressMode::REPEAT;
		sc.magFilter = Filter::LINEAR;
		sc.magFilter = Filter::LINEAR;
		this->sampler = ctx.CreateSampler(sc);
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


			ResourceCreationContext::ImageViewCreateInfo ivc = {};
			ivc.components.r = ComponentSwizzle::R;
			ivc.components.g = ComponentSwizzle::G;
			ivc.components.b = ComponentSwizzle::B;
			ivc.components.a = ComponentSwizzle::A;
			ivc.format = Format::RGBA8;
			ivc.image = this->img;
			ivc.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
			ivc.subresourceRange.baseArrayLayer = 0;
			ivc.subresourceRange.baseMipLevel = 0;
			ivc.subresourceRange.layerCount = 1;
			ivc.subresourceRange.levelCount = 1;
			ivc.viewType = ImageViewHandle::Type::TYPE_2D;
			this->view = ctx.CreateImageView(ivc);

			ResourceCreationContext::SamplerCreateInfo sc = {};
			sc.addressModeU = AddressMode::REPEAT;
			sc.addressModeV = AddressMode::REPEAT;
			sc.magFilter = Filter::LINEAR;
			sc.magFilter = Filter::LINEAR;
			this->sampler = ctx.CreateSampler(sc);
		});
	} else {
		this->img = std::get<ImageHandle *>(info.data);
	}
}

Image::Image(ResourceManager * resMan, std::string const& name, std::vector<uint8_t> const& input) : resMan(resMan)
{
	this->name = name;
	int n;
	uint8_t const * imageData = stbi_load_from_memory((stbi_uc const *)&input[0], (int)input.size(), (int *)&width, (int *)&height, (int *)&n, 4);
	data = std::vector<uint8_t>(width * height * 4);
	memcpy(&data[0], imageData, width * height * 4);
	Format format = Format::RGBA8;
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

		ResourceCreationContext::ImageViewCreateInfo ivc = {};
		ivc.components.r = ComponentSwizzle::R;
		ivc.components.g = ComponentSwizzle::G;
		ivc.components.b = ComponentSwizzle::B;
		ivc.components.a = ComponentSwizzle::A;
		ivc.format = Format::RGBA8;
		ivc.image = this->img;
		ivc.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
		ivc.subresourceRange.baseArrayLayer = 0;
		ivc.subresourceRange.baseMipLevel = 0;
		ivc.subresourceRange.layerCount = 1;
		ivc.subresourceRange.levelCount = 1;
		ivc.viewType = ImageViewHandle::Type::TYPE_2D;
		this->view = ctx.CreateImageView(ivc);

		ResourceCreationContext::SamplerCreateInfo sc = {};
		sc.addressModeU = AddressMode::REPEAT;
		sc.addressModeV = AddressMode::REPEAT;
		sc.magFilter = Filter::LINEAR;
		sc.magFilter = Filter::LINEAR;
		this->sampler = ctx.CreateSampler(sc);
	});
}

std::vector<uint8_t> const& Image::GetData() const
{
	return data;
}

uint32_t Image::GetHeight() const
{
	return height;
}

ImageHandle * Image::GetImage()
{
	return img;
}

ImageViewHandle * Image::GetImageView()
{
	return view;
}

SamplerHandle * Image::GetSampler()
{
	return sampler;
}

uint32_t Image::GetWidth() const
{
	return width;
}
