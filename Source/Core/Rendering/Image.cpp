#include "Core/Rendering/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/ResourceManager.h"
#include "Core/Semaphore.h"

Image::~Image()
{
	ResourceManager::CreateResources([this](ResourceCreationContext& ctx) {
		ctx.DestroyImage(img);
	});
}

Image * Image::FromFile(std::string const& fileName, bool forceReload) {
	if (!forceReload && ResourceManager::GetResource<Image>(fileName)) {
		return ResourceManager::GetResource<Image>(fileName);
	}

	int width, height, n;
	FILE * file = fopen(fileName.c_str(), "rb");
	uint8_t const * imageData = stbi_load_from_file(file, &width, &height, &n, 4);
	std::vector<uint8_t> data(width * height * 4);
	memcpy(&data[0], imageData, width * height * 4);

	Image * ret;
	Semaphore sem;
	ResourceManager::CreateResources([&ret, &sem, data, fileName, width, height](ResourceCreationContext& ctx) {
		ResourceCreationContext::ImageCreateInfo ic = {
			Format::RGBA8,
			ImageHandle::Type::TYPE_2D,
			width, height, 1,
			1,
			IMAGE_USAGE_FLAG_SAMPLED_BIT | IMAGE_USAGE_FLAG_TRANSFER_DST_BIT
		};
		auto img = ctx.CreateImage(ic);
		ctx.ImageData(img, data);

		ret = new Image(fileName, width, height, img);
		ResourceManager::AddResource(fileName, ret);
		// TODO: This should actually be async and return a future
		sem.Signal();
	});
	sem.Wait();
	return ret;
}

uint32_t Image::GetHeight() const
{
	return height;
}

uint32_t Image::GetWidth() const
{
	return width;
}

ImageViewHandle * Image::GetDefaultView()
{
	if (defaultView != nullptr) {
		return defaultView;
	}

	Semaphore sem;
	ResourceManager::CreateResources([this, &sem](ResourceCreationContext& ctx) {
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
		this->defaultView = ctx.CreateImageView(ivc);
		sem.Signal();
	});
	sem.Wait();
	return defaultView;
}

ImageHandle * Image::GetImage() const
{
	return img;
}

Image::Image(std::string const & fileName, uint32_t width, uint32_t height, ImageHandle * img) : fileName(fileName), width(width), height(height), img(img) {}
