#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/ResourceManager.h"

Image::Image(ResourceManager * resMan, std::string const& name) noexcept
{
	this->name = name;
	this->data_ = std::vector<uint8_t>(128 * 128 * 4, 0xFF);

	//resMan->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			128, 128, 1,
			1,
			IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
		};
		printf("pre createimage\n");
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data_);
		this->img_ = tmp;
		printf("post createimage\n");
		//})));
	});
}

Image::Image(ImageCreateInfo const& info) noexcept
{
	this->name = info.name;
	this->width_ = info.width;
	this->height_ = info.height;
	if (info.data.index() == 0) {
		this->data_ = std::get<std::vector<uint8_t>>(info.data);
		info.resMan->CreateResources([this](ResourceCreationContext& ctx) {
			//info.resMan->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
			ResourceCreationContext::ImageCreateInfo ic = {
				Format::RGBA8,
				ImageHandle::Type::TYPE_2D,
				this->width_, this->height_, 1,
				1,
				IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
			};
			printf("pre createimage\n");
			auto tmp = ctx.CreateImage(ic);
			ctx.ImageData(tmp, this->data_);
			this->img_ = tmp;
			printf("post createimage\n");
			//})));
		});
	} else {
		this->img_ = std::get<ImageHandle *>(info.data);
	}
}

Image::Image(ResourceManager * resMan, std::string const& name, std::vector<uint8_t> const& input) noexcept
{
	this->name = name;
	int n;
	uint8_t const * imageData = stbi_load_from_memory((stbi_uc const *)&input[0], input.size(), (int *)&width_, (int *)&height_, (int *)&n, 0);
	data_ = std::vector<uint8_t>(width_ * height_ * n);
	memcpy(&data_[0], imageData, width_ * height_ * n);
	Format format = Format::RGBA8;
#if 0
	switch (n) {
	case 1:
		format = Format::R8;
		break;
	case 2: {
		format = Format::RG8;
		break;
	}
	case 3:
		format = Format::RGB8;
		break;
	case 4:
		format = Format::RGBA8;
		break;
	default:
		printf("[ERROR] Loading image %s: Unknown format %d.\n", name.c_str(), n);
		break;
	}
#endif
	//TODO: pads to RGBA8 in a terrible way
	{
		auto idx = 1;
		auto pad = data_.size() % 4;
		for (auto it = data_.begin(); it != data_.end(); ++it) {
			if (idx % 4 == pad) {
				for (auto i = 0; i < 4 - pad; ++i) {
					it = data_.insert(++it, 0xFF);
					idx++;
				}
			} else {
				idx++;
			}
		}
	}
	//TODO:
	resMan->CreateResources([this](ResourceCreationContext& ctx) {
		//resMan->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			this->width_, this->height_, 1,
			1,
			IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
		};
		printf("pre createimage\n");
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data_);
		this->img_ = tmp;
		printf("post createimage\n");
		//})));
	});
}

std::vector<uint8_t> const& Image::get_data() const noexcept
{
	return data_;
}

uint32_t Image::get_height() const noexcept
{
	return height_;
}

ImageHandle * Image::get_image_handle()
{
	return img_;
}

uint32_t Image::get_width() const noexcept
{
	return width_;
}
