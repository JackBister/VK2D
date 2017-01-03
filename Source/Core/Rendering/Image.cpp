#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/ResourceManager.h"

Image::Image(ResourceManager * resMan, const std::string& name) noexcept
{
	this->name = name;
	this->format = Format::RGBA8;
	this->height = 128;
	this->width = 128;
	this->data = std::vector<uint8_t>(128 * 128 * 4, 0xFF);

	RenderCommand rc(RenderCommand::AddImageParams(this));
	resMan->PushRenderCommand(rc);
}

Image::Image(const ImageCreateInfo& info) noexcept
{
	this->name = info.name;
	this->format = info.format;
	this->height = info.height;
	this->width = info.width;
	this->params = info.params;
	if (info.data.index() == 0) {
		this->data = std::experimental::get<std::vector<uint8_t>>(info.data);
		RenderCommand rc(RenderCommand::AddImageParams(this));
		info.resMan->PushRenderCommand(rc);
	} else {
		this->rendererData = std::experimental::get<ImageRendererData>(info.data);
	}
}

Image::Image(ResourceManager * resMan, const std::string& name, const std::vector<char>& input) noexcept
{
	this->name = name;
	int n;
	const uint8_t * imageData = stbi_load_from_memory((const stbi_uc *)&input[0], input.size(), &width, &height, &n, 0);
	data = std::vector<uint8_t>(width * height * n);
	memcpy(&data[0], imageData, width * height * n);
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
	RenderCommand rc(RenderCommand::AddImageParams(this));
	resMan->PushRenderCommand(rc);
}

void Image::SetRendererData(const ImageRendererData& rData) noexcept
{
	rendererData = rData;
}

const std::vector<uint8_t>& Image::GetData() const noexcept
{
	return data;
}

Image::Format Image::GetFormat() const noexcept
{
	return format;
}

int Image::GetHeight() const noexcept
{
	return height;
}

const Image::ImageParameters& Image::GetParameters() const noexcept
{
	return params;
}

int Image::GetWidth() const noexcept
{
	return width;
}

const ImageRendererData& Image::GetRendererData() const noexcept
{
	return rendererData;
}
