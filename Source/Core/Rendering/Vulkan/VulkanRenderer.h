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

class ResourceManager;

class Renderer : IRenderer
{
	friend class VulkanRenderContext;
	friend class VulkanRenderCommandContext;
	friend class VulkanResourceContext;
public:
	Renderer(ResourceManager *, Queue<RenderCommand>::Reader&&, char const * title, int winX, int winY, int w, int h, uint32_t flags);
	~Renderer() noexcept;

	uint32_t GetSwapCount() final override;

	uint64_t GetFrameTime() noexcept;
	void DrainQueue() noexcept;

	bool isAborting = false;
	int abortCode = 0;

	SDL_Window * window;

	//Total time between swapbuffers
	float frameTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	//Time spent on GPU in a frame
	GLuint timeQuery;

	bool swap;

	static VulkanFramebufferHandle Backbuffer;

private:
	struct VulkanSwapchain
	{
		VkSwapchainKHR swapchain;
		VkFormat format;
		VkExtent2D extent;

		std::vector<VkFramebuffer> framebuffers;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
	};

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CopyBufferToBuffer(VkBuffer src, VkBuffer dst, size_t dstOffset, size_t size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	//Picture related variables
	float aspect_ratio_;
	glm::ivec2 dimensions_;

	//Systems interaction related variables
	ResourceManager * resource_manager_;
	Queue<RenderCommand>::Reader render_queue_;
	Semaphore * swap_sem_;

	//Vulkan specific variables

	VkDescriptorPool vk_descriptor_pool_;

	VkDevice vk_device_;
	VkInstance vk_instance_;
	VkPhysicalDevice vk_physical_device_;
	VkSurfaceKHR vk_surface_;
	VulkanSwapchain vk_swapchain_;
	VkImage vk_backbuffer_image_;

	uint32_t vk_queue_graphics_idx_;
	VkQueue vk_queue_graphics_;
	uint32_t vk_queue_present_idx_;
	VkQueue vk_queue_present_;
	
	VkCommandPool vk_pool_graphics_;
	VkCommandPool vk_pool_present_;

	VkRenderPass vk_render_pass_;

	std::vector<VkCommandBuffer> vk_buffers_present_;

	std::unordered_map<std::thread::id, VkCommandPool> vk_graphics_pools_;

	//Indicates that we have acquired an image from the swapchain
	VkSemaphore swap_img_available_;
	//Indicates that rendering is finished and that we can present a frame
	VkSemaphore ready_for_present_;

};
#endif