#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Backend/Vulkan/VulkanRenderer.h"

#include <SDL2/SDL_vulkan.h>
#include <optick/optick.h>
#include <stb_image.h>
#include <vulkan/vulkan.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Vulkan/VulkanResourceContext.h"

static const auto logger = Logger::Create("VulkanRenderer");
static const auto vulkanLogger = Logger::Create("Vulkan");

static const size_t MIN_STAGING_BUFFER_SIZE = 2 * 1024 * 1024;

static Format ToAbstractFormat(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Format::B8G8R8A8_UNORM;
    }
    return Format::RGBA8;
}

#if defined(_DEBUG)
static VkBool32 VKAPI_PTR DbgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                                      uint64_t object, size_t location, int32_t messageCode, const char * pLayerPrefix,
                                      const char * pMessage, void * pUserData)
{
    if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        vulkanLogger->Errorf("(%s): %s", pLayerPrefix, pMessage);
    } else if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        vulkanLogger->Warnf("(%s): %s", pLayerPrefix, pMessage);
    } else {
        vulkanLogger->Infof("(%s): %s", pLayerPrefix, pMessage);
    }
    return false;
}
#endif

static bool CheckVkDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> extensions)
{
    // Get extension count
    uint32_t extensionCount = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        logger->Severef("Unable to get Vulkan device extension count.");
        return false;
    }

    // Get extension names
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()) !=
        VK_SUCCESS) {
        logger->Severef("Unable to get Vulkan extensions.");
        return false;
    }

    // Check if needed extensions exist in enumerated extension names
    for (size_t i = 0; i < extensions.size(); ++i) {
        bool exists = false;
        for (size_t j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(availableExtensions[j].extensionName, extensions[i]) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            logger->Severef("Vulkan extension %s is not supported!", extensions[i]);
            return false;
        }
    }
    return true;
}

static bool CheckVkInstanceExtensions(std::vector<const char *> extensions)
{
    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        logger->Severef("Unable to get Vulkan extension count.");
        return false;
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
        logger->Severef("Unable to get Vulkan extensions.");
        return false;
    }

    for (size_t i = 0; i < extensions.size(); ++i) {
        bool exists = false;
        for (size_t j = 0; j < availableExtensions.size(); ++j) {
            if (strcmp(availableExtensions[j].extensionName, extensions[i]) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            logger->Severef("Vulkan extension %s is not supported!", extensions[i]);
            return false;
        }
    }
    return true;
}

static std::optional<VkPhysicalDevice> ChoosePhysicalDevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
        logger->Severef("Couldn't enumerate physical devices.");
        return {};
    }
    auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevices[0]) != VK_SUCCESS || deviceCount == 0) {
        logger->Severef("Couldn't enumerate physical devices.");
        return {};
    }

    VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceType chosenType = VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE;
    for (auto const & pd : physicalDevices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pd, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            logger->Infof("Discrete GPU detected. deviceName=%s", props.deviceName);
            chosenDevice = pd;
            chosenType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            break;
        }
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            logger->Infof("iGPU detected. deviceName=%s", props.deviceName);
            chosenDevice = pd;
            chosenType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU &&
                   chosenType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            logger->Infof("CPU detected. deviceName=%s", props.deviceName);
            chosenDevice = pd;
            chosenType = VK_PHYSICAL_DEVICE_TYPE_CPU;
        }
    }
    if (chosenDevice == VK_NULL_HANDLE) {
        logger->Infof("chosenDevice is VK_NULL_HANDLE, defaulting to physicalDevices[0]");
        chosenDevice = physicalDevices[0];
    }
    return chosenDevice;
}

static bool CreateVkCommandPool(VkDevice device, uint32_t const queueIdx, VkCommandPool * const ret)
{
    VkCommandPoolCreateInfo poolCreateInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueIdx};
    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, ret) != VK_SUCCESS) {
        logger->Severef("Couldn't create command pool.");
        return false;
    }
    return true;
}

static bool CreateVkSemaphore(VkDevice device, VkSemaphore * const ret)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, ret) != VK_SUCCESS) {
        logger->Severef("Unable to create semaphore.");
        return false;
    }
    return true;
}

Renderer::~Renderer()
{
    SDL_DestroyWindow(window);
}

uint32_t Renderer::AcquireNextFrameIndex(SemaphoreHandle * signalReady, FenceHandle * signalFence)
{
    uint32_t imageIndex;
    auto res = vkAcquireNextImageKHR(
        basics.device,
        swapchain.swapchain,
        0,
        signalReady != nullptr ? ((VulkanSemaphoreHandle *)signalReady)->semaphore : VK_NULL_HANDLE,
        signalFence != nullptr ? ((VulkanFenceHandle *)signalFence)->fence : VK_NULL_HANDLE,
        &imageIndex);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || this->isSwapchainInvalid) {
        return UINT32_MAX;
    }
    assert(res == VK_SUCCESS);
    return imageIndex;
}
std::vector<ImageViewHandle *> Renderer::GetBackbuffers()
{
    std::vector<ImageViewHandle *> ret(swapchain.imageViews.size());
    for (size_t i = 0; i < swapchain.imageViews.size(); ++i) {
        ret[i] = new VulkanImageViewHandle();
        ((VulkanImageViewHandle *)ret[i])->imageView = swapchain.imageViews[i];
    }
    return ret;
}

Format Renderer::GetBackbufferFormat() const
{
    return ToAbstractFormat(swapchain.format);
}

void Renderer::CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    OPTICK_EVENT();
    VulkanResourceContext ctx(this);
    fun(ctx);
}

void Renderer::ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                                    std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
    OPTICK_EVENT();
    ctx->Execute(this, waitSem, signalSem, signalFence);
}

void Renderer::SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem)
{
    OPTICK_EVENT();
    OPTICK_GPU_FLIP(swapchain.swapchain);
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    if (waitSem != nullptr) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &((VulkanSemaphoreHandle *)waitSem)->semaphore;
    }
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    auto res = vkQueuePresentKHR(presentQueue, &presentInfo);
}

glm::ivec2 Renderer::GetResolution() const
{
    // TODO: May not work when window resolution and render resolution are decoupled?
    return glm::ivec2(swapchain.extent.width, swapchain.extent.height);
}

uint32_t Renderer::GetSwapCount() const
{
    return (uint32_t)swapchain.images.size();
}

Renderer::Renderer(char const * title, int winX, int winY, uint32_t flags, RendererConfig config)
    : config(config), window(SDL_CreateWindow(title, winX, winY, config.windowResolution.x, config.windowResolution.y,
                                              flags | SDL_WINDOW_VULKAN))
{
    stbi_set_flip_vertically_on_load(true);
    std::vector<const char *> instanceExtensions;
    {
        unsigned int requiredInstanceExtensionsCount;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &requiredInstanceExtensionsCount, nullptr)) {
            logger->Severef("Unable to get required Vulkan instance extensions.");
            assert(false);
            exit(1);
        }
        instanceExtensions = std::vector<const char *>(requiredInstanceExtensionsCount);
        SDL_Vulkan_GetInstanceExtensions(window, &requiredInstanceExtensionsCount, &instanceExtensions[0]);
    }

#if defined(_DEBUG)
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
    std::vector<const char *> const instanceLayers = {
#if defined(_DEBUG)
        "VK_LAYER_LUNARG_assistant_layer",
        "VK_LAYER_LUNARG_standard_validation",
#endif
#if defined(VULKAN_API_DUMP)
        "VK_LAYER_LUNARG_api_dump"
#endif
    };

    logger->Infof("Creating Vulkan instance.");
    VkInstanceCreateFlags const vulkanFlags = 0;
    VkInstanceCreateInfo const vulkanInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        vulkanFlags,
        nullptr,
        static_cast<uint32_t>(instanceLayers.size()),
        instanceLayers.size() > 0 ? &instanceLayers[0] : nullptr,
        static_cast<uint32_t>(instanceExtensions.size()),
        instanceExtensions.size() > 0 ? &instanceExtensions[0] : nullptr,
    };

    auto res = vkCreateInstance(&vulkanInfo, nullptr, &basics.instance);
    if (res != VK_SUCCESS) {
        logger->Severef("vkCreateInstance failed.");
        assert(false);
        exit(1);
    }

#if defined(_DEBUG)
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(basics.instance, "vkCreateDebugReportCallbackEXT");
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(
        vkGetInstanceProcAddr(basics.instance, "vkDebugReportMessageEXT"));
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(basics.instance, "vkDestroyDebugReportCallbackEXT"));

    VkDebugReportCallbackCreateInfoEXT dbgInfo = {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
        DbgCallback,
        nullptr};

    VkDebugReportCallbackEXT test;
    res = vkCreateDebugReportCallbackEXT(basics.instance, &dbgInfo, nullptr, &test);
    if (res != VK_SUCCESS) {
        logger->Severef("Failed to create debug report callback.");
        assert(false);
        exit(1);
    }
#endif

    if (!SDL_Vulkan_CreateSurface(window, basics.instance, &surface)) {
        logger->Severef("Unable to create Vulkan surface.");
        assert(false);
        exit(1);
    }

    auto physDeviceOptional = ChoosePhysicalDevice(basics.instance);
    if (!physDeviceOptional.has_value()) {
        logger->Severef("ChoosePhysicalDevice failed.");
        assert(false);
        exit(1);
    }
    basics.physicalDevice = physDeviceOptional.value();

    std::vector<const char *> const deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    if (!CheckVkDeviceExtensions(basics.physicalDevice, deviceExtensions)) {
        logger->Severef("CheckVkDeviceExtensions failed.");
        assert(false);
        exit(1);
    }

    // Create device and queues
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(basics.physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0) {
        logger->Severef("No queue families found.");
        assert(false);
        exit(1);
    }

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    std::vector<VkBool32> queuePresentSupport(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(basics.physicalDevice, &queueFamilyCount, &queueFamilyProperties[0]);

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (vkGetPhysicalDeviceSurfaceSupportKHR(basics.physicalDevice, i, surface, &queuePresentSupport[i]) !=
            VK_SUCCESS) {
            logger->Severef("Couldn't get physical device surface support.");
            assert(false);
            exit(1);
        }

        if ((queueFamilyProperties[i].queueCount > 0) &&
            (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            if (graphicsQueueIdx == UINT32_MAX) {
                graphicsQueueIdx = i;
            }

            // This queue family supports both graphics and present - Definitely use it.
            if (queuePresentSupport[i]) {
                graphicsQueueIdx = i;
                presentQueueIdx = i;
                break;
            }
        }
    }

    // No queue supporting both graphics and present - pick out a queue that supports present
    if (presentQueueIdx == UINT32_MAX) {
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queuePresentSupport[i]) {
                presentQueueIdx = i;
            }
        }
    }

    // Either no graphics queue exists or no present queue exists. We can't work with this.
    if (graphicsQueueIdx == UINT32_MAX || presentQueueIdx == UINT32_MAX) {
        logger->Severef("Vulkan: No queues with graphics or present support found.");
        assert(false);
        exit(1);
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<float> queuePriorities = {1.f};

    queueCreateInfos.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                nullptr,
                                0,
                                graphicsQueueIdx,
                                static_cast<uint32_t>(queuePriorities.size()),
                                &queuePriorities[0]});

    // If we're using a separate present queue we need to create it
    if (graphicsQueueIdx != presentQueueIdx) {
        queueCreateInfos.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    nullptr,
                                    0,
                                    presentQueueIdx,
                                    static_cast<uint32_t>(queuePriorities.size()),
                                    &queuePriorities[0]});
    }

    std::vector<const char *> deviceLayers = {
#if defined(_DEBUG)
        "LAYER_LUNARG_standard_validation"
#endif
    };

    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                           nullptr,
                                           0,
                                           static_cast<uint32_t>(queueCreateInfos.size()),
                                           &queueCreateInfos[0],
                                           static_cast<uint32_t>(deviceLayers.size()),
                                           deviceLayers.size() > 0 ? &deviceLayers[0] : nullptr,
                                           static_cast<uint32_t>(deviceExtensions.size()),
                                           deviceExtensions.size() > 0 ? &deviceExtensions[0] : nullptr,
                                           nullptr};

    if (vkCreateDevice(basics.physicalDevice, &deviceCreateInfo, nullptr, &basics.device) != VK_SUCCESS) {
        logger->Severef("Couldn't create device.");
        assert(false);
        exit(1);
    }

    graphicsQueueIdx = graphicsQueueIdx;
    vkGetDeviceQueue(basics.device, graphicsQueueIdx, 0, &graphicsQueue);
    presentQueueIdx = presentQueueIdx;
    // TODO: graphicsQueueIdx == presentQueueIdx => same queue used for present and graphics => ?
    vkGetDeviceQueue(basics.device, presentQueueIdx, 0, &presentQueue);

    // Create command pools
    if (!CreateVkCommandPool(basics.device, graphicsQueueIdx, &graphicsPool)) {
        logger->Severef("Unable to create graphics pool.");
        assert(false);
        exit(1);
    }

    if (!CreateVkCommandPool(basics.device, presentQueueIdx, &presentPool)) {
        logger->Severef("Unable to create present pool.");
        assert(false);
        exit(1);
    }

    // Get required info and create swap chain
    RecreateSwapchain();

    {
        std::vector<VkDescriptorPoolSize> poolSizes(
            {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50}});

        VkDescriptorPoolCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        ci.poolSizeCount = 2;
        ci.pPoolSizes = &poolSizes[0];
        ci.maxSets = 100;

        auto res = vkCreateDescriptorPool(basics.device, &ci, nullptr, &descriptorPool);
        if (res != VK_SUCCESS) {
            logger->Severef("Couldn't create descriptor pool.");
            assert(false);
            exit(1);
        }
    }

    OPTICK_GPU_INIT_VULKAN(&basics.device, &basics.physicalDevice, &graphicsQueue, &graphicsQueueIdx, 1);
}

uint32_t Renderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->basics.physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    logger->Errorf("Couldn't find memory type, typeFilter=%ud properties=%ud", typeFilter, properties);
    assert(false);
    return 0;
}

void Renderer::CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, size_t dstOffset,
                                  size_t size)
{
    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = dstOffset;
    region.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &region);
}

void Renderer::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width,
                                 uint32_t height)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

VkCommandBuffer Renderer::CreateCommandBuffer(VkCommandBufferLevel level)
{
    VkCommandBuffer ret;
    VkCommandBufferAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, graphicsPool, level, 1};
    vkAllocateCommandBuffers(basics.device, &allocateInfo, &ret);
    return ret;
}
VkExtent2D Renderer::GetDesiredExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, RendererConfig cfg)
{
    VkExtent2D desiredExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == -1) {
        desiredExtent = {static_cast<uint32_t>(cfg.windowResolution.x), static_cast<uint32_t>(cfg.windowResolution.y)};
        if (desiredExtent.width < surfaceCapabilities.minImageExtent.width) {
            desiredExtent.width = surfaceCapabilities.minImageExtent.width;
        }
        if (desiredExtent.height < surfaceCapabilities.minImageExtent.height) {
            desiredExtent.height = surfaceCapabilities.minImageExtent.height;
        }
        if (desiredExtent.width > surfaceCapabilities.maxImageExtent.width) {
            desiredExtent.width = surfaceCapabilities.maxImageExtent.width;
        }
        if (desiredExtent.height > surfaceCapabilities.maxImageExtent.height) {
            desiredExtent.height = surfaceCapabilities.maxImageExtent.height;
        }
    }
    return desiredExtent;
}

VkPresentModeKHR Renderer::GetDesiredPresentMode(std::vector<VkPresentModeKHR> presentModes)
{
    VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (config.presentMode == PresentMode::IMMEDIATE) {
        desiredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    } else if (config.presentMode == PresentMode::FIFO) {
        desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    } else if (config.presentMode == PresentMode::MAILBOX) {
        desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    if (std::find(presentModes.begin(), presentModes.end(), desiredPresentMode) == presentModes.end()) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    return desiredPresentMode;
}

VkSurfaceFormatKHR Renderer::GetDesiredSurfaceFormat(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
    VkSurfaceFormatKHR desiredFormat;
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
        desiredFormat = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
    } else {
        desiredFormat = surfaceFormats[0];
        for (auto & format : surfaceFormats) {
            if (format.format == VK_FORMAT_R8G8B8A8_UNORM) {
                desiredFormat = format;
                break;
            }
        }
    }
    return desiredFormat;
}

uint32_t Renderer::GetDesiredNumberOfImages(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
    // Use one more image than minimum in the swap chain if possible
    uint32_t desiredNumberOfImages = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && desiredNumberOfImages > surfaceCapabilities.maxImageCount) {
        desiredNumberOfImages = surfaceCapabilities.maxImageCount;
    }
    return desiredNumberOfImages;
}

GuardedBufferHandle Renderer::GetStagingBuffer(size_t size)
{
    OPTICK_EVENT();
    for (size_t i = 0; i < stagingBuffers.size(); ++i) {
        if (stagingBuffers[i].size >= size && stagingBuffers[i].guard.try_lock()) {
            return {std::lock_guard<std::mutex>(stagingBuffers[i].guard, std::adopt_lock), &stagingBuffers[i].buffer};
        }
    }

    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // We always round up to a multiplier of MIN_STAGING_BUFFER_SIZE
    size_t multiplier = (size + MIN_STAGING_BUFFER_SIZE - 1) / MIN_STAGING_BUFFER_SIZE;
    stagingInfo.size = multiplier * MIN_STAGING_BUFFER_SIZE;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer stagingBuffer;
    auto res = vkCreateBuffer(basics.device, &stagingInfo, nullptr, &stagingBuffer);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements stagingRequirements = {};
    vkGetBufferMemoryRequirements(basics.device, stagingBuffer, &stagingRequirements);

    VkMemoryAllocateInfo stagingAllocateInfo{};
    stagingAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocateInfo.allocationSize = stagingRequirements.size;
    stagingAllocateInfo.memoryTypeIndex = FindMemoryType(
        stagingRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkDeviceMemory stagingMemory;
    res = vkAllocateMemory(basics.device, &stagingAllocateInfo, nullptr, &stagingMemory);
    assert(res == VK_SUCCESS);

    vkBindBufferMemory(basics.device, stagingBuffer, stagingMemory, 0);

    // 20 minutes of fighting the compiler over copy constructors led to this
    VulkanBufferHandle ffs;
    ffs.buffer = stagingBuffer;
    ffs.memory = stagingMemory;
    stagingBuffers.emplace_back(ffs, stagingInfo.size);
    auto idx = stagingBuffers.size() - 1;
    return {std::lock_guard<std::mutex>(stagingBuffers[idx].guard), &stagingBuffers[idx].buffer};
}

void Renderer::InitSurfaceCapabilities()
{
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(basics.physicalDevice, surface, &capabilities.capabilities) !=
        VK_SUCCESS) {
        logger->Severef("Unable to get device surface capabilities.");
        assert(false);
        exit(1);
    }

    uint32_t formatsCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(basics.physicalDevice, surface, &formatsCount, nullptr) != VK_SUCCESS ||
        formatsCount == 0) {
        logger->Severef("Unable to get device surface formats.");
        assert(false);
        exit(1);
    }

    capabilities.formats = std::vector<VkSurfaceFormatKHR>(formatsCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(basics.physicalDevice, surface, &formatsCount, &capabilities.formats[0]) !=
            VK_SUCCESS ||
        formatsCount == 0) {
        logger->Severef("Unable to get device surface formats.");
        assert(false);
        exit(1);
    }

    uint32_t presentModesCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(basics.physicalDevice, surface, &presentModesCount, nullptr) !=
            VK_SUCCESS ||
        presentModesCount == 0) {
        logger->Severef("Unable to get device present modes.");
        assert(false);
        exit(1);
    }

    capabilities.presentModes = std::vector<VkPresentModeKHR>(presentModesCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            basics.physicalDevice, surface, &presentModesCount, &capabilities.presentModes[0]) != VK_SUCCESS ||
        presentModesCount == 0) {
        logger->Severef("Unable to get device present modes.");
        assert(false);
        exit(1);
    }
}

void Renderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format,
                                     VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        logger->Errorf("Error: Unknown image layout transition.");
        assert(false);
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Renderer::RecreateSwapchain()
{
    logger->Infof("Renderer::RecreateSwapchain");
    isSwapchainInvalid = false;
    InitSurfaceCapabilities();
    uint32_t desiredNumberOfImages = GetDesiredNumberOfImages(capabilities.capabilities);
    VkSurfaceFormatKHR desiredFormat = GetDesiredSurfaceFormat(capabilities.formats);
    VkExtent2D desiredExtent = GetDesiredExtent(capabilities.capabilities, config);

    // TRANSFER_DST_BIT required for clearing op
    if (!(capabilities.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        logger->Severef("VK_IMAGE_USAGE_TRANSFER_DST_BIT not supported by surface.");
        assert(false);
        exit(1);
    }
    VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkSurfaceTransformFlagBitsKHR desiredTransform = capabilities.capabilities.currentTransform;
    if (capabilities.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    VkPresentModeKHR desiredPresentMode = GetDesiredPresentMode(capabilities.presentModes);

    if (desiredPresentMode == VK_PRESENT_MODE_END_RANGE_KHR) {
        logger->Severef("Required present modes not supported.");
        assert(false);
        exit(1);
    }

    // TODO: old swap chain
    auto oldSwapchain = swapchain.swapchain;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                    nullptr,
                                                    0,
                                                    surface,
                                                    desiredNumberOfImages,
                                                    desiredFormat.format,
                                                    desiredFormat.colorSpace,
                                                    desiredExtent,
                                                    1,
                                                    desiredUsage,
                                                    VK_SHARING_MODE_EXCLUSIVE,
                                                    0,
                                                    nullptr,
                                                    desiredTransform,
                                                    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                    desiredPresentMode,
                                                    true,
                                                    oldSwapchain};

    if (vkCreateSwapchainKHR(basics.device, &swapchainCreateInfo, nullptr, &swapchain.swapchain) != VK_SUCCESS) {
        logger->Severef("Couldn't create swap chain.");
        assert(false);
        exit(1);
    }
    swapchain.extent = desiredExtent;
    swapchain.format = desiredFormat.format;

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(basics.device, oldSwapchain, nullptr);
    }

    uint32_t swapchainImageCount = 0;
    auto res = vkGetSwapchainImagesKHR(basics.device, swapchain.swapchain, &swapchainImageCount, nullptr);
    if (res != VK_SUCCESS || swapchainImageCount == 0) {
        logger->Severef("Couldn't get swap chain image count.");
        assert(false);
        exit(1);
    }

    swapchain.images.clear();
    swapchain.images.resize(swapchainImageCount);
    res = vkGetSwapchainImagesKHR(basics.device, swapchain.swapchain, &swapchainImageCount, &swapchain.images[0]);
    if (res != VK_SUCCESS || swapchainImageCount == 0) {
        logger->Severef("Couldn't get swap chain images.");
        assert(false);
        exit(1);
    }

    for (auto iv : swapchain.imageViews) {
        vkDestroyImageView(basics.device, iv, nullptr);
    }

    swapchain.imageViews.clear();
    swapchain.imageViews.resize(swapchain.images.size());
    for (uint32_t i = 0; i < swapchain.images.size(); ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        res = vkCreateImageView(basics.device, &createInfo, nullptr, &swapchain.imageViews[i]);
        if (res != VK_SUCCESS) {
            logger->Severef("Couldn't create swap chain image views.");
            assert(false);
            exit(1);
        }
    }
}

RendererConfig Renderer::GetConfig()
{
    return this->config;
}

void Renderer::UpdateConfig(RendererConfig config)
{
    this->config = config;
    SDL_SetWindowSize(window, config.windowResolution.x, config.windowResolution.y);
    this->isSwapchainInvalid = true;
}

#endif
