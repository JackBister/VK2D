#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "render.h"

Image::Image(const std::string& name) noexcept
{
	this->name = name;
	this->format = Format::RGBA8;
	this->height = 128;
	this->width = 128;
	this->data = std::vector<uint8_t>(128 * 128 * 4, 0xFF);
	Render_currentRenderer->AddImage(this);
}

Image::Image(const std::string& name, const std::vector<char>& input) noexcept
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
	Render_currentRenderer->AddImage(this);
}

void Image::SetRendererData(void * p) noexcept
{
	rendererData = p;
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

int Image::GetWidth() const noexcept
{
	return width;
}

void * Image::GetRendererData() const noexcept
{
	return rendererData;
}
