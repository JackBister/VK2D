#include "Image.h"

#include <optick/optick.h>
#define STB_IMAGE_IMPLEMENTATION
#include <ThirdParty/stb/stb_image.h>

#include "Core/Resources/ResourceManager.h"
#include "Logging/Logger.h"
#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Abstract/ResourceCreationContext.h"
#include "Util/Semaphore.h"

#if HOT_RELOAD_RESOURCES
#include "Util/WatchFile.h"
#endif

static const auto logger = Logger::Create("Image");

struct ImageAndView {
    ImageHandle * image;
    ImageViewHandle * imageView;
};

static ImageAndView CreateImageResources(std::vector<uint8_t> const & data, int width, int height)
{
    OPTICK_EVENT();
    ImageAndView ret;
    Semaphore sem;
    ResourceManager::CreateResources([&ret, &sem, &data, width, height](ResourceCreationContext & ctx) {
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

bool CheckForTransparency(std::vector<uint8_t> const & data)
{
    OPTICK_EVENT();
    auto size = data.size();
    if (size >= 4) {
        for (size_t i = 3; i < size; i += 4) {
            if (data[i] != 0xFF) {
                return true;
            }
        }
    }
    return false;
}

static std::optional<std::vector<uint8_t>> ReadImageFile(std::string fileName, int * width, int * height)
{
    OPTICK_EVENT();
    int n;
    FILE * file = fopen(fileName.c_str(), "rb");
    if (!file) {
        logger.Error("Got error when opening fileName='{}'", fileName);
        *width = 0;
        *height = 0;
        return std::nullopt;
    }
    uint8_t * imageData = stbi_load_from_file(file, width, height, &n, 4);
    fclose(file);
    std::vector<uint8_t> data(*width * *height * 4);
    memcpy(&data[0], imageData, *width * *height * 4);
    stbi_image_free(imageData);
    return data;
}

Image::Image(std::string const & fileName, uint32_t width, uint32_t height, bool hasTransparency, ImageHandle * img,
             ImageViewHandle * defaultView)
    : fileName(fileName), width(width), height(height), hasTransparency(hasTransparency), img(img),
      defaultView(defaultView)
{
    logger.Info("Image {} hasTransparency {}", fileName, hasTransparency);
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

Image * Image::FromData(std::string const & filename, uint32_t width, uint32_t height, std::vector<uint8_t> data)
{
    OPTICK_EVENT();
    auto imageAndView = CreateImageResources(data, width, height);
    auto ret =
        new Image(filename, width, height, CheckForTransparency(data), imageAndView.image, imageAndView.imageView);
    ResourceManager::AddResource(filename, ret);
    ResourceManager::AddResource(filename + "/defaultView.imageview", ret->defaultView);
    return ret;
}

Image * Image::FromFile(std::string const & fileName, bool forceReload)
{
    OPTICK_EVENT();
    if (!forceReload && ResourceManager::GetResource<Image>(fileName)) {
        return ResourceManager::GetResource<Image>(fileName);
    }

    int width, height;
    auto dataOpt = ReadImageFile(fileName, &width, &height);
    if (!dataOpt.has_value()) {
        return nullptr;
    }
    auto data = dataOpt.value();

    auto imageAndView = CreateImageResources(data, width, height);
    logger.Info("Initial load '{}' image={}, imageView={}", fileName, imageAndView.image, imageAndView.imageView);
    auto ret =
        new Image(fileName, width, height, CheckForTransparency(data), imageAndView.image, imageAndView.imageView);
    ResourceManager::AddResource(fileName, ret);
    ResourceManager::AddResource(fileName + "/defaultView.imageview", ret->defaultView);

#if HOT_RELOAD_RESOURCES
    WatchFile(fileName, [fileName]() {
        logger.Info("Image file '{}' changed, will reload", fileName);
        auto image = ResourceManager::GetResource<Image>(fileName);
        if (image == nullptr) {
            logger.Warn("Image file '{}' was changed but ResourceManager had no reference for it.", fileName);
            return;
        }
        int width, height;
        auto dataOpt = ReadImageFile(fileName, &width, &height);
        if (!dataOpt.has_value()) {
            logger.Warn("Failed to read image file '{}'", fileName);
            return;
        }
        auto data = dataOpt.value();
        auto previousImage = image->img;
        auto previousView = image->defaultView;
        auto imageAndView = CreateImageResources(data, width, height);
        image->width = width;
        image->height = height;
        image->img = imageAndView.image;
        image->defaultView = imageAndView.imageView;
        logger.Info("Reload '{}' image={}, imageView={}", fileName, imageAndView.image, imageAndView.imageView);
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

bool Image::HasTransparency() const
{
    return hasTransparency;
}

ImageViewHandle * Image::GetDefaultView() const
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
