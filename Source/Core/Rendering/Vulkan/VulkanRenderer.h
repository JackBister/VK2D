#ifdef USE_VULKAN_RENDERER
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
#include "Core/Rendering/Vulkan/VulkanRenderCommandContext.h"
#include "Core/Rendering/Vulkan/VulkanRenderContext.h"

class Renderer : IRenderer
{
	friend class VulkanRenderContext;
	friend class VulkanRenderCommandContext;
	friend class VulkanResourceContext;
public:
	Renderer(/*ResourceManager *, Queue<RenderCommand>::Reader&&,*/ char const * title, int winX, int winY, int w, int h, uint32_t flags);
	~Renderer() noexcept;

	uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
	std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) final override;
	Format GetBackbufferFormat() final override;
	//FramebufferHandle * GetBackbuffer() final override;
	uint32_t GetSwapCount() final override;

	void CreateResources(std::function<void(ResourceCreationContext&)> fun) final override;
	void ExecuteCommandContext(RenderCommandContext * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence = nullptr) final override;
	void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) final override;

	//void Swap(uint32_t imageIndex, SemaphoreHandle * waitSem);

	//void DrainQueue() noexcept;

	bool isAborting = false;
	int abortCode = 0;

	SDL_Window * window;

private:
	struct VulkanBasics
	{
		VkDevice vk_device_;
		VkInstance vk_instance_;
		VkPhysicalDevice vk_physical_device_;
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

	//Picture related variables
	float aspectRatio;
	glm::ivec2 dimensions;

	VulkanBasics basics;

	VkDescriptorPool vk_descriptor_pool_;

	VkSurfaceKHR vk_surface_;
	VulkanSwapchain vk_swapchain_;

	uint32_t vk_queue_graphics_idx_;
	VkQueue vk_queue_graphics_;
	uint32_t vk_queue_present_idx_;
	VkQueue vk_queue_present_;
	
	VkCommandPool vk_pool_graphics_;
	VkCommandPool vk_pool_present_;
};
#endif