#include "Core/Rendering/Vulkan/VulkanRenderer.h"

#include <SDL/SDL_vulkan.h>
#include <stb_image.h>
#include <vulkan/vulkan.h>


#if defined(_DEBUG)
static VkBool32 VKAPI_PTR DbgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char * pLayerPrefix, const char * pMessage, void * pUserData)
{
	printf("[%s] %s\n", pLayerPrefix, pMessage);
	return false;
}
#endif

static bool CheckVkDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> extensions)
{
	//Get extension count
	uint32_t extensionCount = 0;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan device extension count.\n");
		return false;
	}

	//Get extension names
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan extensions.\n");
		return false;
	}

	//Check if needed extensions exist in enumerated extension names
	for (size_t i = 0; i < extensions.size(); ++i) {
		bool exists = false;
		for (size_t j = 0; j < availableExtensions.size(); ++j) {
			if (strcmp(availableExtensions[j].extensionName, extensions[i]) == 0) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			printf("[ERROR] Vulkan extension %s is not supported!\n", extensions[i]);
			return false;
		}
	}
	return true;
}

static bool CheckVkInstanceExtensions(std::vector<const char *> extensions)
{
	uint32_t extensionCount = 0;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan extension count.\n");
		return false;
	}

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan extensions.\n");
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
			printf("[ERROR] Vulkan extension %s is not supported!\n", extensions[i]);
			return false;
		}
	}
	return true;
}

static std::optional<VkPhysicalDevice> ChoosePhysicalDevice(VkInstance instance) {
	uint32_t deviceCount = 0;
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
		printf("[ERROR] Vulkan: Couldn't enumerate physical devices.\n");
		return {};
	}
	auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevices[0]) != VK_SUCCESS || deviceCount == 0) {
		printf("[ERROR] Vulkan: Couldn't enumerate physical devices.\n");
		return {};
	}

	VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceType chosenType;
	for (auto const& pd : physicalDevices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(pd, &props);
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			printf("[INFO] Vulkan: Discrete GPU detected. %s\n", props.deviceName);
			chosenDevice = pd;
			chosenType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			break;
		}
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			printf("[INFO] Vulkan: iGPU detected. %s\n", props.deviceName);
			chosenDevice = pd;
			chosenType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
		} else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU && chosenType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			printf("[INFO] Vulkan: CPU detected. %s\n", props.deviceName);
			chosenDevice = pd;
			chosenType = VK_PHYSICAL_DEVICE_TYPE_CPU;
		}
	}
	if (chosenDevice == VK_NULL_HANDLE) {
		printf("[TRACE] Vulkan: chosenDevice is VK_NULL_HANDLE, defaulting...\n");
		chosenDevice = physicalDevices[0];
	}
	return chosenDevice;
}

static bool CreateVkCommandPool(VkDevice device, uint32_t const queueIdx, VkCommandPool * const ret)
{
	VkCommandPoolCreateInfo poolCreateInfo = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		queueIdx
	};
	if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, ret) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Couldn't create command pool.\n");
		return false;
	}
	return true;
}

static bool CreateVkSemaphore(VkDevice device, VkSemaphore * const ret)
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};
	if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, ret) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Unable to create semaphore.\n");
		return false;
	}
	return true;
}

Renderer::~Renderer()
{
	SDL_DestroyWindow(window);
}

std::unique_ptr<VulkanRenderCommandContext> Renderer::CreateCommandContext()
{
	return std::make_unique<VulkanRenderCommandContext>(new std::allocator<uint8_t>());
}

Renderer::Renderer(ResourceManager * resMan, Queue<RenderCommand>::Reader&& reader, Semaphore * sem, char const * title, int winX, int winY, int w, int h, uint32_t flags)
	: aspect_ratio_(static_cast<float>(w) / static_cast<float>(h)),
	dimensions_(w, h),
	render_queue_(std::move(reader)),
	resource_manager_(resMan),
	swap_sem_(sem),
	window(SDL_CreateWindow(title, winX, winY, w, h, flags | SDL_WINDOW_VULKAN))
{
	stbi_set_flip_vertically_on_load(true);

	std::vector<const char *> const instanceExtensions {
#if defined(_DEBUG)
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME,
		//TODO: Other WMs
#if defined(USE_PLATFORM_WIN32_KHR)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
	};

	std::vector<const char *> const instanceLayers = {
#if defined(_DEBUG)
		"LAYER_LUNARG_standard_validation",
#endif
#if defined(VULKAN_API_DUMP)
		"LAYER_LUNARG_api_dump"
#endif
	};

	printf("[INFO] %s:%d Creating Vulkan instance.\n", __FILE__, __LINE__);
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

	if (vkCreateInstance(&vulkanInfo, nullptr, &vk_instance_) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Failed to create instance\n");
		throw std::exception("vkCreateInstance failed.");
	}

#if defined(_DEBUG)
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(vk_instance_, "vkCreateDebugReportCallbackEXT"));
	PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(vk_instance_, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(vk_instance_, "vkDestroyDebugReportCallbackEXT"));

	VkDebugReportCallbackCreateInfoEXT dbgInfo = {
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		nullptr,
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		DbgCallback,
		nullptr
	};

	VkDebugReportCallbackEXT test;
	if (vkCreateDebugReportCallbackEXT(vk_instance_, &dbgInfo, nullptr, &test) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Failed to create debug report callback.\n");
		throw std::exception("vkCreateDebugReportCallbackEXT failed.");
	}
#endif

	if (!SDL_Vulkan_CreateSurface(window, vk_instance_, &vk_surface_)) {
		throw std::exception("Unable to create Vulkan surface.");
	}

	auto physDeviceOptional = ChoosePhysicalDevice(vk_instance_);
	if (!physDeviceOptional.has_value()) {
		throw std::exception("ChoosePhysicalDevice failed.");
	}
	vk_physical_device_ = physDeviceOptional.value();


	std::vector<const char*> const deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (!CheckVkDeviceExtensions(vk_physical_device_, deviceExtensions)) {
		throw std::exception("CheckVkDeviceExtensions failed.");
	}

	/*
	Create device and queues
	*/
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queueFamilyCount, nullptr);
	if (queueFamilyCount == 0) {
		throw std::exception("No queue families found.\n");
	}

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	std::vector<VkBool32> queuePresentSupport(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queueFamilyCount, &queueFamilyProperties[0]);

	uint32_t graphicsQueueIdx = UINT32_MAX;
	uint32_t presentQueueIdx = UINT32_MAX;

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device_, i, vk_surface_, &queuePresentSupport[i]) != VK_SUCCESS) {
			throw std::exception("Couldn't get physical device surface support.\n");
		}

		if ((queueFamilyProperties[i].queueCount > 0) && (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			if (graphicsQueueIdx == UINT32_MAX) {
				graphicsQueueIdx = i;
			}

			//This queue family supports both graphics and present - Definitely use it.
			if (queuePresentSupport[i]) {
				graphicsQueueIdx = i;
				presentQueueIdx = i;
				break;
			}
		}
	}

	//No queue supporting both graphics and present - pick out a queue that supports present
	if (presentQueueIdx == UINT32_MAX) {
		for (uint32_t i = 0; i < queueFamilyCount; ++i) {
			if (queuePresentSupport[i]) {
				presentQueueIdx = i;
			}
		}
	}

	//Either no graphics queue exists or no present queue exists. We can't work with this.
	if (graphicsQueueIdx == UINT32_MAX || presentQueueIdx == UINT32_MAX) {
		printf("[ERROR] Vulkan: No queues with graphics or present support found.\n");
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<float> queuePriorities = { 1.f };

	queueCreateInfos.push_back({
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		graphicsQueueIdx,
		static_cast<uint32_t>(queuePriorities.size()),
		&queuePriorities[0]
	});

	//If we're using a separate present queue we need to create it
	if (graphicsQueueIdx != presentQueueIdx) {
		queueCreateInfos.push_back({
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			presentQueueIdx,
			static_cast<uint32_t>(queuePriorities.size()),
			&queuePriorities[0]
		});
	}

	std::vector<const char *> deviceLayers = {
#if defined(_DEBUG)
		"LAYER_LUNARG_standard_validation"
#endif
	};

	VkDeviceCreateInfo deviceCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>(queueCreateInfos.size()),
		&queueCreateInfos[0],
		static_cast<uint32_t>(deviceLayers.size()),
		deviceLayers.size() > 0 ? &deviceLayers[0] : nullptr,
		static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.size() > 0 ? &deviceExtensions[0] : nullptr,
		nullptr
	};

	if (vkCreateDevice(vk_physical_device_, &deviceCreateInfo, nullptr, &vk_device_) != VK_SUCCESS) {
		throw std::exception("Couldn't create device.\n");
	}

	vkGetDeviceQueue(vk_device_, graphicsQueueIdx, 0, &vk_queue_graphics_);
	vkGetDeviceQueue(vk_device_, presentQueueIdx, 0, &vk_queue_present_);

	if (!CreateVkSemaphore(vk_device_, &swap_img_available_)) {
		throw std::exception("Couldn't create swap_img_available_ semaphore.\n");
	}

	if (!CreateVkSemaphore(vk_device_, &ready_for_present_)) {
		throw std::exception("Couldn't create ready_for_present_ semaphore.\n");
	}

	/*
	Get required info and create swap chain
	*/
	//{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_, &surfaceCapabilities) != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface capabilities.\n");
	}

	uint32_t formatsCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_, &formatsCount, nullptr) != VK_SUCCESS || formatsCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface formats.\n");
	}

	auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(formatsCount);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device_, vk_surface_, &formatsCount, &surfaceFormats[0]) != VK_SUCCESS || formatsCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface formats.\n");
	}

	uint32_t presentModesCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &presentModesCount, nullptr) != VK_SUCCESS || presentModesCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device present modes.\n");
	}

	auto presentModes = std::vector<VkPresentModeKHR>(presentModesCount);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &presentModesCount, &presentModes[0]) != VK_SUCCESS || presentModesCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device present modes.\n");
	}

	//Use one more image than minimum in the swap chain if possible
	uint32_t desiredNumberOfImages = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && desiredNumberOfImages > surfaceCapabilities.maxImageCount) {
		desiredNumberOfImages = surfaceCapabilities.maxImageCount;
	}

	VkSurfaceFormatKHR desiredFormat;
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		desiredFormat = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	} else {
		desiredFormat = surfaceFormats[0];
		for (auto& format : surfaceFormats) {
			if (format.format == VK_FORMAT_R8G8B8A8_UNORM) {
				desiredFormat = format;
				break;
			}
		}
	}

	VkExtent2D desiredExtent = surfaceCapabilities.currentExtent;
	if (surfaceCapabilities.currentExtent.width == -1) {
		desiredExtent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
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

	//TRANSFER_DST_BIT required for clearing op
	if (!(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		throw std::exception("[ERROR] Vulkan: VK_IMAGE_USAGE_TRANSFER_DST_BIT not supported by surface.\n");
	}
	VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSurfaceTransformFlagBitsKHR desiredTransform = surfaceCapabilities.currentTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_END_RANGE_KHR;
	//TODO: Options
	for (auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			desiredPresentMode = presentMode;
			break;
		}
		if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
			desiredPresentMode = presentMode;
		}
		if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR && desiredPresentMode != VK_PRESENT_MODE_FIFO_KHR) {
			desiredPresentMode = presentMode;
		}
	}

	if (desiredPresentMode == VK_PRESENT_MODE_END_RANGE_KHR) {
		throw std::exception("[ERROR] Vulkan: Required present modes not supported.\n");
	}

	//TODO: old swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,
		0,
		vk_surface_,
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
		VK_NULL_HANDLE
	};

	if (vkCreateSwapchainKHR(vk_device_, &swapchainCreateInfo, nullptr, &vk_swapchain_.swapchain) != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Can't create swap chain.\n");
	}
	vk_swapchain_.extent = desiredExtent;
	vk_swapchain_.format = desiredFormat.format;

	//	}
	/*
	Create command pools
	*/
	if (!CreateVkCommandPool(vk_device_, graphicsQueueIdx, &vk_pool_graphics_)) {
		throw std::exception("Unable to create graphics pool.");
	}

	if (!CreateVkCommandPool(vk_device_, presentQueueIdx, &vk_pool_present_)) {
		throw std::exception("Unable to create present pool.");
	}

	/*
	Create present command buffers
	*/
		uint32_t swapchainImageCount = 0;
		if (vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_.swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS || swapchainImageCount == 0) {
			throw std::exception("[ERROR] Vulkan: Couldn't get swap chain image count.");
		}
		vk_buffers_present_.resize(swapchainImageCount);

		VkCommandBufferAllocateInfo presentCmdbufInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			vk_pool_present_,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			swapchainImageCount
		};
		if (vkAllocateCommandBuffers(vk_device_, &presentCmdbufInfo, vk_buffers_present_.data()) != VK_SUCCESS) {
			throw std::exception("[ERROR] Vulkan: Couldn't allocate present command buffers.");
		}

	/*
	Record present command buffers
	*/
	{
		uint32_t swapchainImageCount = 0;
		if (vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_.swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS || swapchainImageCount == 0) {
			throw std::exception("[ERROR] Vulkan: Couldn't get swap chain image count.\n");
		}
		auto swapchainImages = std::vector<VkImage>(swapchainImageCount);
		if (vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_.swapchain, &swapchainImageCount, &swapchainImages[0]) != VK_SUCCESS) {
			throw std::exception("[ERROR] Vulkan: Couldn't get swap chain images.\n");
		}

		//TODO:
		VkCommandBufferBeginInfo presentBufBeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			nullptr
		};
		VkClearColorValue clearColor = {
			{ 1.0f, 0.8f, 0.4f, 0.0f }
		};
		VkImageSubresourceRange imgSubresourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			1,
			0,
			1
		};

		for (uint32_t i = 0; i < swapchainImageCount; ++i) {
			//Memory barrier that moves from IMAGE_LAYOUT_PRESENT_SOURCE_KHR to IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			VkImageMemoryBarrier barrierFromPresentToClear = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				presentQueueIdx,
				presentQueueIdx,
				swapchainImages[i],
				imgSubresourceRange
			};
			//Memory barrier that moves from IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to IMAGE_LAYOUT_PRESENT_SOURCE_KHR
			VkImageMemoryBarrier barrierFromClearToPresent = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				presentQueueIdx,
				presentQueueIdx,
				swapchainImages[i],
				imgSubresourceRange
			};
			vkBeginCommandBuffer(vk_buffers_present_[i], &presentBufBeginInfo);
			vkCmdPipelineBarrier(vk_buffers_present_[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToClear);
			//TODO: Replace with blit from graphics queue backbuffer?
			vkCmdClearColorImage(vk_buffers_present_[i], swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imgSubresourceRange);
			vkCmdPipelineBarrier(vk_buffers_present_[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClearToPresent);
			if (vkEndCommandBuffer(vk_buffers_present_[i]) != VK_SUCCESS) {
				throw std::exception("Vulkan: Unable to record present command buffer.");
			}
		}
	}

	VkAttachmentDescription attachmentDescriptions[] = {
		{
			0,
			vk_swapchain_.format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	VkAttachmentReference colorAttachmentReferences[] = {
		{
			0,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	};

	VkSubpassDescription subpassDescriptions[] = {
		{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			1,
			colorAttachmentReferences,
			nullptr,
			nullptr,
			0,
			nullptr
		}
	};

	VkRenderPassCreateInfo renderpassCreateInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		0,
		1,
		attachmentDescriptions,
		1,
		subpassDescriptions,
		0,
		nullptr
	};
	if (vkCreateRenderPass(vk_device_, &renderpassCreateInfo, nullptr, &vk_render_pass_) != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Couldn't create render pass.");
	}
}
