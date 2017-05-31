#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/ResourceManager.h"

Image::Image(ResourceManager * resMan, const std::string& name) noexcept
{
	this->name = name;
	this->data = std::vector<uint8_t>(128 * 128 * 4, 0xFF);

	RenderCommand rc(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			128, 128, 1,
			1
		};
		printf("pre createimage\n");
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data);
		this->img = tmp;
		printf("post createimage\n");
	}));
	resMan->PushRenderCommand(rc);
}

Image::Image(const ImageCreateInfo& info) noexcept
{
	this->name = info.name;
	this->width = info.width;
	this->height = info.height;
	if (info.data.index() == 0) {
		this->data = std::get<std::vector<uint8_t>>(info.data);
		RenderCommand rc(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
			ResourceCreationContext::ImageCreateInfo ic = {
				Format::RGBA8,
				ImageHandle::Type::TYPE_2D,
				this->width, this->height, 1,
				1
			};
			printf("pre createimage\n");
			auto tmp = ctx.CreateImage(ic);
			ctx.ImageData(tmp, this->data);
			this->img = tmp;
			printf("post createimage\n");
		}));
		info.resMan->PushRenderCommand(rc);
	} else {
		this->img = std::get<ImageHandle *>(info.data);
	}
}

Image::Image(ResourceManager * resMan, const std::string& name, const std::vector<uint8_t>& input) noexcept
{
	this->name = name;
	int n;
	const uint8_t * imageData = stbi_load_from_memory((const stbi_uc *)&input[0], input.size(), (int *)&width, (int *)&height, &n, 0);
	data = std::vector<uint8_t>(width * height * n);
	memcpy(&data[0], imageData, width * height * n);
	Format format;
	switch (n) {
	case 1:
		format = Format::R8;
		break;
	case 2:
		format = Format::RG8;
		break;
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
	RenderCommand rc(RenderCommand::CreateResourceParams([this](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			this->width, this->height, 1,
			1
		};
		printf("pre createimage\n");
		auto tmp = ctx.CreateImage(ic);
		ctx.ImageData(tmp, this->data);
		this->img = tmp;
		printf("post createimage\n");
	}));
	resMan->PushRenderCommand(rc);
}

const std::vector<uint8_t>& Image::GetData() const noexcept
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
