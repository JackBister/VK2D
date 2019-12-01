#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"

#if HOT_RELOAD_RESOURCES
#include "Core/Util/WatchFile.h"
#endif

static const auto logger = Logger::Create("Image");

struct ImageAndView {
    ImageHandle * image;
    ImageViewHandle * imageView;
};

static ImageAndView CreateImageResources(std::vector<uint8_t> const & data, int width, int height)
{
    ImageAndView ret;
    Semaphore sem;
    ResourceManager::CreateResources([&ret, &sem, data, width, height](ResourceCreationContext & ctx) {
        ResourceCreationContext::ImageCreateInfo ic = {Format::RGBA8,
                                                       ImageHandle::Type::TYPE_2D,
                                                       width,
                                                       height,
                                                       1,
                                                       1,
                                                       IMAGE_USAGE_FLAG_SAMPLED_BIT |
                                                           IMAGE_USAGE_FLAG_TRANSFER_DST_BIT};
        auto img = ctx.CreateImage(ic);
        ctx.ImageData(img, data);

        ResourceCreationContext::ImageViewCreateInfo ivc = {};
        ivc.components.r = ComponentSwizzle::R;
        ivc.components.g = ComponentSwizzle::G;
        ivc.components.b = ComponentSwizzle::B;
        ivc.components.a = ComponentSwizzle::A;
        ivc.format = Format::RGBA8;
        ivc.image = img;
        ivc.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
        ivc.subresourceRange.baseArrayLayer = 0;
        ivc.subresourceRange.baseMipLevel = 0;
        ivc.subresourceRange.layerCount = 1;
        ivc.subresourceRange.levelCount = 1;
        ivc.viewType = ImageViewHandle::Type::TYPE_2D;
        auto defaultView = ctx.CreateImageView(ivc);

        ret = {img, defaultView};
        // TODO: This should actually be async and return a future
        sem.Signal();
    });
    sem.Wait();
    return ret;
}

static std::vector<uint8_t> ReadImageFile(std::string fileName, int * width, int * height)
{
    int n;
    FILE * file = fopen(fileName.c_str(), "rb");
    if (!file) {
        logger->Errorf("Got error when opening fileName='%s'", fileName.c_str());
        *width = 0;
        *height = 0;
        return {};
    }
    uint8_t const * imageData = stbi_load_from_file(file, width, height, &n, 4);
    fclose(file);
    std::vector<uint8_t> data(*width * *height * 4);
    memcpy(&data[0], imageData, *width * *height * 4);
    return data;
}

Image::Image(std::string const & fileName, uint32_t width, uint32_t height, ImageHandle * img,
             ImageViewHandle * defaultView)
    : fileName(fileName), width(width), height(height), img(img), defaultView(defaultView)
{
}

Image::~Image()
{
    auto defaultView = this->defaultView;
    auto img = this->img;
    ResourceManager::DestroyResources([defaultView, img](ResourceCreationContext & ctx) {
        ctx.DestroyImageView(defaultView);
        ctx.DestroyImage(img);
    });
}

Image * Image::FromFile(std::string const & fileName, bool forceReload)
{
    if (!forceReload && ResourceManager::GetResource<Image>(fileName)) {
        return ResourceManager::GetResource<Image>(fileName);
    }

    int width, height;
    auto data = ReadImageFile(fileName, &width, &height);

    auto imageAndView = CreateImageResources(data, width, height);
    logger->Infof(
        "Initial load '%s' image=%p, imageView=%p", fileName.c_str(), imageAndView.image, imageAndView.imageView);
    auto ret = new Image(fileName, width, height, imageAndView.image, imageAndView.imageView);
    ResourceManager::AddResource(fileName, ret);
    ResourceManager::AddResource(fileName + "/defaultView.imageview", ret->defaultView);

#if HOT_RELOAD_RESOURCES
    WatchFile(fileName, [fileName]() {
        logger->Infof("Image file '%s' changed, will reload", fileName.c_str());
        auto image = ResourceManager::GetResource<Image>(fileName);
        if (image == nullptr) {
            logger->Warnf("Image file '%s' was changed but ResourceManager had no reference for it.", fileName.c_str());
            return;
        }
        int width, height;
        auto data = ReadImageFile(fileName, &width, &height);
        auto previousImage = image->img;
        auto previousView = image->defaultView;
        auto imageAndView = CreateImageResources(data, width, height);
        image->width = width;
        image->height = height;
        image->img = imageAndView.image;
        image->defaultView = imageAndView.imageView;
        logger->Infof(
            "Reload '%s' image=%p, imageView=%p", fileName.c_str(), imageAndView.image, imageAndView.imageView);
        for (auto cb : image->hotReloadCallbacks) {
            cb.second(image);
        }

        ResourceManager::DestroyResources([previousImage, previousView](ResourceCreationContext & ctx) {
            ctx.DestroyImageView(previousView);
            ctx.DestroyImage(previousImage);
        });
    });
#endif

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
    return defaultView;
}

ImageHandle * Image::GetImage() const
{
    return img;
}

#if HOT_RELOAD_RESOURCES
int Image::SubscribeToChanges(std::function<void(Image *)> cb)
{
    hotReloadCallbacks.insert_or_assign(hotReloadSubscriberId, cb);
    return hotReloadSubscriberId++;
}

void Image::Unsubscribe(int subscriptionId)
{
    hotReloadCallbacks.erase(subscriptionId);
}
#endif
