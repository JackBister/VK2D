#pragma once
#ifndef USE_OGL_RENDERER
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Core/Queue.h"
#include "Core/Rendering/AbstractRenderer.h"
#include "Core/Rendering/RenderCommand.h"
#include "Core/Rendering/Vulkan/VulkanContextStructs.h"

class Renderer : IRenderer
{
	friend class VulkanResourceContext;
	friend class VulkanCommandBuffer;
	friend class VulkanResourceContext;
public:
	Renderer(char const * title, int winX, int winY, int w, int h, uint32_t flags);
	~Renderer();

	uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
	std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) final override;
	Format GetBackbufferFormat() final override;
	uint32_t GetSwapCount() final override;

	void CreateResources(std::function<void(ResourceCreationContext&)> fun) final override;
	void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence = nullptr) final override;
	void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) final override;

	int abortCode = 0;

private:
	struct VulkanBasics
	{
		VkDevice device;
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
	};
	struct VulkanSwapchain
	{
		VkSwapchainKHR swapchain;
		VkFormat format;
		VkExtent2D extent;

		std::vector<VulkanFramebufferHandle> backbuffers;
		std::vector<VkFramebuffer> framebuffers;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
	};

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, size_t dstOffset, size_t size);
	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level);
	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	float aspectRatio;
	glm::ivec2 dimensions;

	VulkanBasics basics;

	VkDescriptorPool descriptorPool;

	VkSurfaceKHR surface;
	VulkanSwapchain swapchain;

	uint32_t graphicsQueueIdx;
	VkQueue graphicsQueue;
	uint32_t presentQueueIdx;
	VkQueue presentQueue;
	
	VkCommandPool graphicsPool;
	VkCommandPool presentPool;

	SDL_Window * window;
};
#endif