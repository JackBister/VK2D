#pragma once
#ifndef USE_OGL_RENDERER
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>
#include <gl/glew.h>
#include <vulkan/vulkan.h>

#include "../Abstract/AbstractRenderer.h"
#include "Util/Queue.h"
#include "VulkanContextStructs.h"

struct CommandBufferAndFence {
    VkCommandBuffer commandBuffer;
    VkFence isReady;
};

struct GuardedBufferHandle {
    std::lock_guard<std::mutex> guard;
    VulkanBufferHandle * buffer;
    size_t size;
};

struct GuardedDescriptorPool {
    std::mutex poolLock;
    VkDescriptorPool pool;
};

struct GuardedQueue {
    std::mutex queueLock;
    VkQueue queue;
};

struct LockedQueue {
    std::lock_guard<std::mutex> queueLock;
    VkQueue queue;
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
    Renderer(char const * title, int winX, int winY, uint32_t flags, RendererConfig config, int numThreads);
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

    RendererProperties const & GetProperties() final override;

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
    VkExtent2D GetDesiredExtent(VkSurfaceCapabilitiesKHR, RendererConfig);
    VkPresentModeKHR GetDesiredPresentMode(std::vector<VkPresentModeKHR>);
    VkSurfaceFormatKHR GetDesiredSurfaceFormat(std::vector<VkSurfaceFormatKHR>);
    uint32_t GetDesiredNumberOfImages(VkSurfaceCapabilitiesKHR);
    CommandBufferAndFence GetGraphicsStagingCommandBuffer();
    GuardedBufferHandle GetStagingBuffer(size_t size);
    CommandBufferAndFence GetStagingCommandBuffer();
    LockedQueue GetGraphicsQueue();
    LockedQueue GetTransferQueue();
    void InitSurfaceCapabilities();
    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout);

    struct FrameInfo {
        // All these vectors should have length = NumWorkerThreads after constructor is done
        std::vector<VkCommandPool> graphicsCommandPools;
        std::vector<std::vector<CommandBufferAndFence>> graphicsCommandBuffers;
        std::vector<VkCommandPool> transferCommandPools;
        std::vector<std::vector<CommandBufferAndFence>> transferCommandBuffers;
    };

    uint32_t currFrameInfoIdx = 0;
    uint32_t prevFrameInfoIdx = 0;
    // length = swapchain length
    std::vector<FrameInfo> frameInfos;

    RendererConfig config;
    RendererProperties properties;

    // TODO: This isn't very elegant
    bool isSwapchainInvalid = false;

    VulkanBasics basics;
    SurfaceCapabilities capabilities;
    VkPhysicalDeviceFeatures supportedFeatures;

    // length = NumWorkerThreads
    std::deque<GuardedDescriptorPool> descriptorPools;

    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;

    std::deque<GuardedQueue> graphicsQueues;
    std::deque<GuardedQueue> transferQueues;

    uint32_t graphicsQueueIdx;

    uint32_t presentQueueIdx;
    VkQueue presentQueue;

    uint32_t transferFamilyIdx;

    std::deque<VulkanStagingBuffer> stagingBuffers;

    int numThreads;

    SDL_Window * window;
};
#endif