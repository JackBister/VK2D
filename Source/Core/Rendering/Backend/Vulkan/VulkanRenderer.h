#pragma once
#ifndef USE_OGL_RENDERER
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Core/Queue.h"
#include "Core/Rendering/Backend/Abstract/AbstractRenderer.h"
#include "Core/Rendering/Backend/Vulkan/VulkanContextStructs.h"

struct GuardedBufferHandle {
    std::lock_guard<std::mutex> guard;
    VulkanBufferHandle * buffer;
};

struct VulkanStagingBuffer {
    VulkanStagingBuffer(VulkanBufferHandle buffer, size_t size) : buffer(buffer), size(size) {}

    VulkanBufferHandle buffer;
    size_t size;
    std::mutex guard;
};

struct VulkanStagingCommandBuffer {
    VulkanStagingCommandBuffer(VkFence inUse, VkCommandBuffer commandBuffer)
        : inUse(inUse), commandBuffer(commandBuffer)
    {
    }

    std::mutex guard;
    VkFence inUse;
    VkCommandBuffer commandBuffer;
};

struct GuardedStagingCommandBuffer {
    std::lock_guard<std::mutex> guard;
    VkFence inUse;
    VkCommandBuffer commandBuffer;
};

class Renderer : IRenderer
{
    friend class VulkanResourceContext;
    friend class VulkanCommandBuffer;
    friend class VulkanResourceContext;

public:
    Renderer(char const * title, int winX, int winY, uint32_t flags, RendererConfig config);
    ~Renderer();

    uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
    std::vector<ImageViewHandle *> GetBackbuffers() final override;
    Format GetBackbufferFormat() const final override;
    glm::ivec2 GetResolution() const final override;
    uint32_t GetSwapCount() const final override;

    void CreateResources(std::function<void(ResourceCreationContext &)> fun) final override;
    void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                              std::vector<SemaphoreHandle *> signalSem,
                              FenceHandle * signalFence = nullptr) final override;
    void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) final override;

    void RecreateSwapchain() final override;

    RendererConfig GetConfig() final override;
    void UpdateConfig(RendererConfig) final override;

    int abortCode = 0;

private:
    struct SurfaceCapabilities {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    struct VulkanBasics {
        VkDevice device;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
    };
    struct VulkanSwapchain {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat format;
        VkExtent2D extent;

        std::vector<VulkanFramebufferHandle> backbuffers;
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
    };

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, size_t dstOffset, size_t size);
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);
    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level);
    VkExtent2D GetDesiredExtent(VkSurfaceCapabilitiesKHR, RendererConfig);
    VkPresentModeKHR GetDesiredPresentMode(std::vector<VkPresentModeKHR>);
    VkSurfaceFormatKHR GetDesiredSurfaceFormat(std::vector<VkSurfaceFormatKHR>);
    uint32_t GetDesiredNumberOfImages(VkSurfaceCapabilitiesKHR);
    GuardedBufferHandle GetStagingBuffer(size_t size);
    GuardedStagingCommandBuffer GetStagingCommandBuffer();
    void InitSurfaceCapabilities();
    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout);

    RendererConfig config;
    // TODO: This isn't very elegant
    bool isSwapchainInvalid = false;

    VulkanBasics basics;
    SurfaceCapabilities capabilities;
    VkPhysicalDeviceFeatures supportedFeatures;

    VkDescriptorPool descriptorPool;

    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;

    uint32_t graphicsQueueIdx;
    VkQueue graphicsQueue;
    uint32_t presentQueueIdx;
    VkQueue presentQueue;

    VkCommandPool graphicsPool;
    VkCommandPool presentPool;

    std::deque<VulkanStagingBuffer> stagingBuffers;
    std::deque<VulkanStagingCommandBuffer> stagingCommandBuffers;

    SDL_Window * window;
};
#endif