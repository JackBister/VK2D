#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Vulkan/VulkanRenderer.h"

#include <SDL/SDL_vulkan.h>
#include <stb_image.h>
#include <vulkan/vulkan.h>

VulkanFramebufferHandle Renderer::Backbuffer;

static Format ToAbstractFormat(VkFormat format) {
	switch (format) {
	case VK_FORMAT_B8G8R8A8_UNORM:
		return Format::B8G8R8A8_UNORM;
	}
	return Format::RGBA8;
}

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
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
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

#if 0
std::unique_ptr<VulkanRenderCommandContext> Renderer::CreateCommandContext(RenderCommandContextCreateInfo * pCreateInfo)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	switch (pCreateInfo->level) {
	case RenderCommandContextLevel::PRIMARY: {
		auto cb = getAvailablePrimaryCommandBuffers.Pop();
		if (!cb.has_value()) {
			return nullptr;
		}
		auto ret = cb.value();
		vkBeginCommandBuffer(ret, &beginInfo);
		return std::make_unique<VulkanRenderCommandContext>(ret, new std::allocator<uint8_t>());
	}
	case RenderCommandContextLevel::SECONDARY: {
		auto cb = getAvailableSecondaryCommandBuffers.Pop();
		if (!cb.has_value()) {
			return nullptr;
		}
		auto ret = cb.value();
		vkBeginCommandBuffer(ret, &beginInfo);
		return std::make_unique<VulkanRenderCommandContext>(ret, new std::allocator<uint8_t>());
	}
	}
	throw std::exception("Unknown render command context level.");
}
#endif

Renderer::Renderer(ResourceManager * resMan, Queue<RenderCommand>::Reader&& reader, char const * title, int winX, int winY, int w, int h, uint32_t flags)
	: aspect_ratio_(static_cast<float>(w) / static_cast<float>(h)),
	dimensions_(w, h),
	render_queue_(std::move(reader)),
	resource_manager_(resMan),
	window(SDL_CreateWindow(title, winX, winY, w, h, flags | SDL_WINDOW_VULKAN))
{
	stbi_set_flip_vertically_on_load(true);

	std::vector<const char *> instanceExtensions;
	{
		unsigned int requiredInstanceExtensionsCount;
		if (!SDL_Vulkan_GetInstanceExtensions(window, &requiredInstanceExtensionsCount, nullptr)) {
			throw std::exception("Unable to get required Vulkan instance extensions.");
		}
		instanceExtensions = std::vector<const char *>(requiredInstanceExtensionsCount);
		SDL_Vulkan_GetInstanceExtensions(window, &requiredInstanceExtensionsCount, &instanceExtensions[0]);
	}

#if defined(_DEBUG)
	instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
		instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		//TODO: Other WMs
#if defined(USE_PLATFORM_WIN32_KHR)
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	std::vector<const char *> const instanceLayers = {
#if defined(_DEBUG)
		"VK_LAYER_LUNARG_standard_validation"
#endif
#if defined(VULKAN_API_DUMP)
		"VK_LAYER_LUNARG_api_dump"
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

	auto res = vkCreateInstance(&vulkanInfo, nullptr, &vk_instance_);
	if (res != VK_SUCCESS) {
		throw std::exception("vkCreateInstance failed.");
	}

#if defined(_DEBUG)
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
	(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vk_instance_, "vkCreateDebugReportCallbackEXT");
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
	res = vkCreateDebugReportCallbackEXT(vk_instance_, &dbgInfo, nullptr, &test);
	if (res != VK_SUCCESS) {
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

	vk_queue_graphics_idx_ = graphicsQueueIdx;
	vkGetDeviceQueue(vk_device_, graphicsQueueIdx, 0, &vk_queue_graphics_);
	vk_queue_present_idx_ = presentQueueIdx;
	vkGetDeviceQueue(vk_device_, presentQueueIdx, 0, &vk_queue_present_);

	/*
	Create command pools
	*/
	if (!CreateVkCommandPool(vk_device_, graphicsQueueIdx, &vk_pool_graphics_)) {
		throw std::exception("Unable to create graphics pool.");
	}
	vk_graphics_pools_[std::this_thread::get_id()] = vk_pool_graphics_;

	if (!CreateVkCommandPool(vk_device_, presentQueueIdx, &vk_pool_present_)) {
		throw std::exception("Unable to create present pool.");
	}

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

	uint32_t swapchainImageCount = 0;
	res = vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_.swapchain, &swapchainImageCount, nullptr);
	if (res != VK_SUCCESS || swapchainImageCount == 0) {
		throw std::exception("[ERROR] Vulkan: Couldn't get swap chain image count.");
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
			VK_IMAGE_LAYOUT_UNDEFINED,
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
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		1,
		attachmentDescriptions,
		1,
		subpassDescriptions,
		0,
		nullptr
	};
	res = vkCreateRenderPass(vk_device_, &renderpassCreateInfo, nullptr, &vk_render_pass_);
	if (res != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Couldn't create render pass.");
	}

	vk_swapchain_.images.resize(swapchainImageCount);
	res = vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_.swapchain, &swapchainImageCount, &vk_swapchain_.images[0]);
	if (res != VK_SUCCESS || swapchainImageCount == 0) {
		throw std::exception("[ERROR] Vulkan: Couldn't get swap chain images");
	}

	{
		VkImageCreateInfo abstractBackbufferInfo = {};
		abstractBackbufferInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		abstractBackbufferInfo.imageType = VK_IMAGE_TYPE_2D;
		abstractBackbufferInfo.format = desiredFormat.format;
		abstractBackbufferInfo.extent = {desiredExtent.width, desiredExtent.height, 1};
		abstractBackbufferInfo.mipLevels = 1;
		abstractBackbufferInfo.arrayLayers = 1;
		abstractBackbufferInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		abstractBackbufferInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		abstractBackbufferInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		abstractBackbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkImage abstractBackbuffer;
		res = vkCreateImage(vk_device_, &abstractBackbufferInfo, nullptr, &abstractBackbuffer);
		if (res != VK_SUCCESS) {
			throw std::exception("[ERROR] Vulkan: Couldn't get swap chain images");
		}

		VkMemoryRequirements const memoryRequirements = [this](VkImage img) {
			VkMemoryRequirements ret{0};
			vkGetImageMemoryRequirements(vk_device_, img, &ret);
			return ret;
		}(abstractBackbuffer);

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(vk_physical_device_, &memoryProperties);

		auto memoryIndex = FindMemoryType(memoryRequirements.memoryTypeBits, 0);

		VkMemoryAllocateInfo const info{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memoryRequirements.size,
			memoryIndex
		};

		VkDeviceMemory const memory = [this](VkMemoryAllocateInfo const& info) {
			VkDeviceMemory ret{0};
			auto const res = vkAllocateMemory(vk_device_, &info, nullptr, &ret);
			assert(res == VK_SUCCESS);
			return ret;
		}(info);

		res = vkBindImageMemory(vk_device_, abstractBackbuffer, memory, 0);

		VkBufferCreateInfo stagingInfo = {};
		stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingInfo.size = memoryRequirements.size;
		stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkBuffer stagingBuffer;
		auto res = vkCreateBuffer(vk_device_, &stagingInfo, nullptr, &stagingBuffer);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements stagingRequirements = {};
		vkGetBufferMemoryRequirements(vk_device_, stagingBuffer, &stagingRequirements);

		VkMemoryAllocateInfo stagingAllocateInfo{};
		stagingAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		stagingAllocateInfo.allocationSize = stagingRequirements.size;
		stagingAllocateInfo.memoryTypeIndex = FindMemoryType(stagingRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VkDeviceMemory stagingMemory;
		res = vkAllocateMemory(vk_device_, &stagingAllocateInfo, nullptr, &stagingMemory);
		assert(res == VK_SUCCESS);

		vkBindBufferMemory(vk_device_, stagingBuffer, stagingMemory, 0);

		uint8_t * const mappedMemory = [this](VkDeviceMemory const& memory, size_t size) {
			uint8_t * ret = nullptr;
			auto const res = vkMapMemory(vk_device_, memory, 0, size, 0, (void **)&ret);
			assert(res == VK_SUCCESS);
			return ret;
		}(stagingMemory, stagingRequirements.size);

		for (size_t i = 0; i < memoryRequirements.size; ++i) {
			mappedMemory[i] = 0.5f;
		}

		vkUnmapMemory(vk_device_, stagingMemory);

		TransitionImageLayout(abstractBackbuffer, desiredFormat.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, abstractBackbuffer, desiredExtent.width, desiredExtent.height);

		vkDestroyBuffer(vk_device_, stagingBuffer, nullptr);
		vkFreeMemory(vk_device_, stagingMemory, nullptr);

		vk_swapchain_.images.push_back(abstractBackbuffer);
	}
	
	vk_swapchain_.imageViews.resize(vk_swapchain_.images.size());
	vk_swapchain_.framebuffers.resize(vk_swapchain_.images.size());
	for (uint32_t i = 0; i < vk_swapchain_.images.size(); ++i) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = vk_swapchain_.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = vk_swapchain_.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		res = vkCreateImageView(vk_device_, &createInfo, nullptr, &vk_swapchain_.imageViews[i]);
		if (res != VK_SUCCESS) {
			throw std::exception("Couldn't create swap chain image views.");
		}
		VkFramebufferCreateInfo framebufferInfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			vk_render_pass_,
			1,
			&vk_swapchain_.imageViews[i],
			vk_swapchain_.extent.width,
			vk_swapchain_.extent.height,
			1
		};
		res = vkCreateFramebuffer(vk_device_, &framebufferInfo, nullptr, &vk_swapchain_.framebuffers[i]);
		if (res != VK_SUCCESS) {
			throw std::exception("Couldn't create swap chain framebuffers.");
		}
	}
	Backbuffer.framebuffer_ = vk_swapchain_.framebuffers[vk_swapchain_.framebuffers.size() - 1];
	Backbuffer.format = ToAbstractFormat(vk_swapchain_.format);
	vk_backbuffer_image_ = vk_swapchain_.images[vk_swapchain_.images.size() - 1];
	vk_swapchain_.images.pop_back();
	vk_swapchain_.imageViews.pop_back();
	vk_swapchain_.framebuffers.pop_back();



#if 0
	/*
	Create present command buffers
	*/
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
#endif

	
	{
		std::vector<VkDescriptorPoolSize> poolSizes({
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50},
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50}
		});

		VkDescriptorPoolCreateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		ci.poolSizeCount = 2;
		ci.pPoolSizes = &poolSizes[0];
		ci.maxSets = 100;

		auto res = vkCreateDescriptorPool(vk_device_, &ci, nullptr, &vk_descriptor_pool_);
		if (res != VK_SUCCESS) {
			throw std::exception("Vulkan: Couldn't create descriptor pool.");
		}
	}

	{
		TransitionImageLayout(vk_backbuffer_image_, vk_swapchain_.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		uint32_t imageIndex;
		auto res = vkAcquireNextImageKHR(vk_device_, vk_swapchain_.swapchain, std::numeric_limits<uint64_t>::max(), swap_img_available_, VK_NULL_HANDLE, &imageIndex);
		//TODO: check res

		auto cb = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cb, &beginInfo);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = vk_swapchain_.images[imageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				cb,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		VkImageCopy imgCopy = {};
		imgCopy.dstOffset = {0, 0, 0};
		imgCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgCopy.dstSubresource.baseArrayLayer = 0;
		imgCopy.dstSubresource.layerCount = 1;
		imgCopy.dstSubresource.mipLevel = 0;
		imgCopy.extent = {vk_swapchain_.extent.width, vk_swapchain_.extent.height, 1};
		imgCopy.srcOffset = {0, 0, 0};
		imgCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgCopy.srcSubresource.baseArrayLayer = 0;
		imgCopy.srcSubresource.layerCount = 1;
		imgCopy.srcSubresource.mipLevel = 0;
		vkCmdCopyImage(cb, vk_backbuffer_image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk_swapchain_.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = vk_swapchain_.images[imageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				cb,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		//TODO: !!!
		vkEndCommandBuffer(cb);
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cb;
		submitInfo.waitSemaphoreCount = 1;
		VkSemaphore waitSems[] = { swap_img_available_ };
		submitInfo.pWaitSemaphores = waitSems;
		VkPipelineStageFlags stageMasks[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
		submitInfo.pWaitDstStageMask = stageMasks;
		//TODO: !!!
		submitInfo.signalSemaphoreCount = 1;
		VkSemaphore signalSems[] = {ready_for_present_};
		submitInfo.pSignalSemaphores = signalSems;
		vkQueueSubmit(vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &ready_for_present_;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &vk_swapchain_.swapchain;
		presentInfo.pImageIndices = &imageIndex;

		res = vkQueuePresentKHR(vk_queue_present_, &presentInfo);

		vkQueueWaitIdle(vk_queue_graphics_);
		vkQueueWaitIdle(vk_queue_present_);
	}
}


void Renderer::DrainQueue() noexcept
{
	using namespace std::literals::chrono_literals;
	RenderCommand command = render_queue_.Wait();
	switch (command.params.index()) {
	case RenderCommand::Type::NOP:
		printf("[WARNING] NOP render command.\n");
		break;
	case RenderCommand::Type::ABORT:
		isAborting = true;
		abortCode = std::get<RenderCommand::AbortParams>(command.params).errorCode;
		break;
	case RenderCommand::Type::CREATE_RESOURCE:
	{
		auto fun = std::get<RenderCommand::CreateResourceParams>(command.params).fun;
		VulkanResourceContext ctx(this);
		fun(ctx);
		break;
	}
	case RenderCommand::Type::EXECUTE_COMMAND_CONTEXT:
	{
		auto params = std::get<RenderCommand::ExecuteCommandContextParams>(command.params);
		params.ctx->Execute(this, params.waitSem, params.signalSem);
		break;
	}
	case RenderCommand::Type::SWAP_WINDOW:
	{
		auto params = std::get<RenderCommand::SwapWindowParams>(command.params);
		uint32_t imageIndex;
		auto res = vkAcquireNextImageKHR(vk_device_, vk_swapchain_.swapchain, std::numeric_limits<uint64_t>::max(), swap_img_available_, VK_NULL_HANDLE, &imageIndex);
		//TODO: check res

		auto cb = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cb, &beginInfo);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = vk_swapchain_.images[imageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				cb,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		VkImageCopy imgCopy = {};
		imgCopy.dstOffset = {0, 0, 0};
		imgCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgCopy.dstSubresource.baseArrayLayer = 0;
		imgCopy.dstSubresource.layerCount = 1;
		imgCopy.dstSubresource.mipLevel = 0;
		imgCopy.extent = {vk_swapchain_.extent.width, vk_swapchain_.extent.height, 1};
		imgCopy.srcOffset = {0, 0, 0};
		imgCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgCopy.srcSubresource.baseArrayLayer = 0;
		imgCopy.srcSubresource.layerCount = 1;
		imgCopy.srcSubresource.mipLevel = 0;
		vkCmdCopyImage(cb, vk_backbuffer_image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk_swapchain_.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = vk_swapchain_.images[imageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				cb,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		//TODO: !!!
		vkEndCommandBuffer(cb);
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cb;
		submitInfo.waitSemaphoreCount = 2;
		VkSemaphore waitSems[] = { swap_img_available_, ((VulkanSemaphoreHandle *)params.waitSem)->semaphore_ };
		submitInfo.pWaitSemaphores = waitSems;
		VkPipelineStageFlags stageMasks[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		submitInfo.pWaitDstStageMask = stageMasks;
		//TODO: !!!
		submitInfo.signalSemaphoreCount = 2;
		VkSemaphore signalSems[] = {ready_for_present_, ((VulkanSemaphoreHandle *)params.signalSem)->semaphore_ };
		submitInfo.pSignalSemaphores = signalSems;
		vkQueueSubmit(vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &ready_for_present_;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &vk_swapchain_.swapchain;
		presentInfo.pImageIndices = &imageIndex;

		res = vkQueuePresentKHR(vk_queue_present_, &presentInfo);

		vkQueueWaitIdle(vk_queue_graphics_);
		vkQueueWaitIdle(vk_queue_present_);

		break;
	}
	/*
	case RenderCommand::Type::ADD_BUFFER:
	{
	AddBuffer(std::get<RenderCommand::AddBufferParams>(command.params));
	break;
	}
	case RenderCommand::Type::DELETE_BUFFER:
	DeleteBuffer(std::get<RenderCommand::DeleteBufferParams>(command.params).rData);
	break;
	case RenderCommand::Type::ADD_FRAMEBUFFER:
	AddFramebuffer(std::get<RenderCommand::AddFramebufferParams>(command.params).fb);
	break;
	case RenderCommand::Type::DELETE_FRAMEBUFFER:
	DeleteFramebuffer(std::get<RenderCommand::DeleteFramebufferParams>(command.params).fb);
	break;
	case RenderCommand::Type::ADD_PROGRAM:
	AddProgram(std::get<RenderCommand::AddProgramParams>(command.params).prog);
	break;
	case RenderCommand::Type::DELETE_PROGRAM:
	DeleteProgram(std::get<RenderCommand::DeleteProgramParams>(command.params).prog);
	break;
	case RenderCommand::Type::ADD_SHADER:
	AddShader(std::get<RenderCommand::AddShaderParams>(command.params).shader);
	break;
	case RenderCommand::Type::DELETE_SHADER:
	DeleteShader(std::get<RenderCommand::DeleteShaderParams>(command.params).shader);
	break;
	*/
	default:
		printf("[WARNING] Unimplemented render command: %zu\n", command.params.index());
	}
#if 0
	if (swap) {
		auto preSwap = std::chrono::high_resolution_clock::now();
		auto res = preSwap - lastTime;
		auto avgSwapTime = totalSwapTime / frameCount;
		if (res + avgSwapTime < 8.333334ms) {
			std::this_thread::sleep_for((totalSwapTime / frameCount) - 0.5ms);
		}
		//With Aero enabled in Windows 7, this doesn't busy wait.
		//With Aero disabled, it does, causing 100% use on the GPU thread. Crazy.
		SDL_GL_SwapWindow(window);
		//TODO: These are both necessary to reduce CPU usage
		//My understanding is that without these SwapWindow returns instantly and spins up its own thread(?) that spinlocks, borking our totalSwapTime. 
		glFlush();
		glFinish();
		swap_sem_->Signal();
		swap = false;
		auto postSwap = std::chrono::high_resolution_clock::now();
		frameTime = static_cast<float>((postSwap - lastTime).count());
		lastTime = postSwap;

		frameCount++;
		totalSwapTime += std::chrono::duration_cast<std::chrono::milliseconds>(postSwap - preSwap);
	}
#endif
}

uint32_t Renderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(this->vk_physical_device_, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type.");
}

void Renderer::CopyBufferToBuffer(VkBuffer src, VkBuffer dst, size_t dstOffset, size_t size)
{
	auto commandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = dstOffset;
	region.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &region);
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk_queue_graphics_);
}

void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	auto commandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk_queue_graphics_);
}

VkCommandBuffer Renderer::CreateCommandBuffer(VkCommandBufferLevel level)
{
	auto threadIdx = std::this_thread::get_id();
	if (vk_graphics_pools_.find(threadIdx) == vk_graphics_pools_.end()) {
		VkCommandPool newPool;
		assert(CreateVkCommandPool(vk_device_, vk_queue_graphics_idx_, &newPool));
		vk_graphics_pools_[threadIdx] = newPool;
	}
	VkCommandBuffer ret;
	VkCommandBufferAllocateInfo allocateInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		vk_graphics_pools_[threadIdx],
		level,
		1
	};
	vkAllocateCommandBuffers(vk_device_, &allocateInfo, &ret);
	return ret;
}

void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	auto commandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
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
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
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
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk_queue_graphics_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk_queue_graphics_);
}

#endif
