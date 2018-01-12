#ifndef USE_OGL_RENDERER
#include "Core/Rendering/Vulkan/VulkanRenderer.h"

#include <SDL2/SDL_vulkan.h>
#include <stb_image.h>
#include <vulkan/vulkan.h>

#include "Core/Rendering/Vulkan/VulkanResourceContext.h"

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
	VkPhysicalDeviceType chosenType = VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE;
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

uint32_t Renderer::AcquireNextFrameIndex(SemaphoreHandle * signalReady, FenceHandle * signalFence)
{
	uint32_t imageIndex;
	auto res = vkAcquireNextImageKHR(basics.device, swapchain.swapchain, std::numeric_limits<uint64_t>::max(), signalReady != nullptr ? ((VulkanSemaphoreHandle *)signalReady)->semaphore : VK_NULL_HANDLE, signalFence != nullptr ? ((VulkanFenceHandle *)signalFence)->fence : VK_NULL_HANDLE, &imageIndex);
	assert(res == VK_SUCCESS);
	return imageIndex;
}

std::vector<FramebufferHandle *> Renderer::CreateBackbuffers(RenderPassHandle * renderPass)
{
	swapchain.backbuffers.resize(swapchain.images.size());
	std::vector<FramebufferHandle *> ret(swapchain.backbuffers.size());
	for (size_t i = 0; i < swapchain.images.size(); ++i) {
		VkFramebufferCreateInfo framebufferInfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			((VulkanRenderPassHandle *)renderPass)->renderPass,
			1,
			&swapchain.imageViews[i],
			swapchain.extent.width,
			swapchain.extent.height,
			1
		};
		auto res = vkCreateFramebuffer(basics.device, &framebufferInfo, nullptr, &swapchain.framebuffers[i]);
		if (res != VK_SUCCESS) {
			throw std::exception("Couldn't create swap chain framebuffers.");
		}

		VulkanFramebufferHandle handle = {};
		//TODO:
		handle.attachmentCount = 0;
		handle.format = ToAbstractFormat(swapchain.format);
		handle.framebuffer = swapchain.framebuffers[i];
		handle.height = swapchain.extent.height;
		handle.width = swapchain.extent.width;
		handle.layers = 1;
		handle.pAttachments = nullptr;
		swapchain.backbuffers[i] = handle;
		ret[i] = &swapchain.backbuffers[i];
	}
	return ret;
}

Format Renderer::GetBackbufferFormat()
{
	return ToAbstractFormat(swapchain.format);
}

void Renderer::CreateResources(std::function<void(ResourceCreationContext&)> fun)
{
	VulkanResourceContext ctx(this);
	fun(ctx);
}

void Renderer::ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
	ctx->Execute(this, waitSem, signalSem, signalFence);
}

void Renderer::SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem)
{
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

uint32_t Renderer::GetSwapCount()
{
	return (uint32_t)swapchain.images.size();
}

Renderer::Renderer(char const * title, int winX, int winY, int w, int h, uint32_t flags)
	: aspectRatio(static_cast<float>(w) / static_cast<float>(h)),
	dimensions(w, h),
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

	auto res = vkCreateInstance(&vulkanInfo, nullptr, &basics.instance);
	if (res != VK_SUCCESS) {
		throw std::exception("vkCreateInstance failed.");
	}

#if defined(_DEBUG)
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(basics.instance, "vkCreateDebugReportCallbackEXT");
	PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(basics.instance, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(basics.instance, "vkDestroyDebugReportCallbackEXT"));

	VkDebugReportCallbackCreateInfoEXT dbgInfo = {
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		nullptr,
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		DbgCallback,
		nullptr
	};

	VkDebugReportCallbackEXT test;
	res = vkCreateDebugReportCallbackEXT(basics.instance, &dbgInfo, nullptr, &test);
	if (res != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Failed to create debug report callback.\n");
		throw std::exception("vkCreateDebugReportCallbackEXT failed.");
	}
#endif

	if (!SDL_Vulkan_CreateSurface(window, basics.instance, &surface)) {
		throw std::exception("Unable to create Vulkan surface.");
	}

	auto physDeviceOptional = ChoosePhysicalDevice(basics.instance);
	if (!physDeviceOptional.has_value()) {
		throw std::exception("ChoosePhysicalDevice failed.");
	}
	basics.physicalDevice = physDeviceOptional.value();

	std::vector<const char*> const deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (!CheckVkDeviceExtensions(basics.physicalDevice, deviceExtensions)) {
		throw std::exception("CheckVkDeviceExtensions failed.");
	}

	//Create device and queues
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(basics.physicalDevice, &queueFamilyCount, nullptr);
	if (queueFamilyCount == 0) {
		throw std::exception("No queue families found.\n");
	}

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	std::vector<VkBool32> queuePresentSupport(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(basics.physicalDevice, &queueFamilyCount, &queueFamilyProperties[0]);

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (vkGetPhysicalDeviceSurfaceSupportKHR(basics.physicalDevice, i, surface, &queuePresentSupport[i]) != VK_SUCCESS) {
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
		throw std::exception("Vulkan: No queues with graphics or present support found.\n");
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<float> queuePriorities = {1.f};

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

	if (vkCreateDevice(basics.physicalDevice, &deviceCreateInfo, nullptr, &basics.device) != VK_SUCCESS) {
		throw std::exception("Couldn't create device.\n");
	}

	graphicsQueueIdx = graphicsQueueIdx;
	vkGetDeviceQueue(basics.device, graphicsQueueIdx, 0, &graphicsQueue);
	presentQueueIdx = presentQueueIdx;
	vkGetDeviceQueue(basics.device, presentQueueIdx, 0, &presentQueue);

	//Create command pools
	if (!CreateVkCommandPool(basics.device, graphicsQueueIdx, &graphicsPool)) {
		throw std::exception("Unable to create graphics pool.");
	}

	if (!CreateVkCommandPool(basics.device, presentQueueIdx, &presentPool)) {
		throw std::exception("Unable to create present pool.");
	}

	//Get required info and create swap chain
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(basics.physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface capabilities.\n");
	}

	uint32_t formatsCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(basics.physicalDevice, surface, &formatsCount, nullptr) != VK_SUCCESS || formatsCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface formats.\n");
	}

	auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(formatsCount);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(basics.physicalDevice, surface, &formatsCount, &surfaceFormats[0]) != VK_SUCCESS || formatsCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device surface formats.\n");
	}

	uint32_t presentModesCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(basics.physicalDevice, surface, &presentModesCount, nullptr) != VK_SUCCESS || presentModesCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device present modes.\n");
	}

	auto presentModes = std::vector<VkPresentModeKHR>(presentModesCount);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(basics.physicalDevice, surface, &presentModesCount, &presentModes[0]) != VK_SUCCESS || presentModesCount == 0) {
		throw std::exception("[ERROR] Vulkan: Unable to get device present modes.\n");
	}

	//Use one more image than minimum in the swap chain if possible
	uint32_t desiredNumberOfImages = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && desiredNumberOfImages > surfaceCapabilities.maxImageCount) {
		desiredNumberOfImages = surfaceCapabilities.maxImageCount;
	}

	VkSurfaceFormatKHR desiredFormat;
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		desiredFormat = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
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
		desiredExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
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
		VK_NULL_HANDLE
	};

	if (vkCreateSwapchainKHR(basics.device, &swapchainCreateInfo, nullptr, &swapchain.swapchain) != VK_SUCCESS) {
		throw std::exception("[ERROR] Vulkan: Can't create swap chain.\n");
	}
	swapchain.extent = desiredExtent;
	swapchain.format = desiredFormat.format;

	uint32_t swapchainImageCount = 0;
	res = vkGetSwapchainImagesKHR(basics.device, swapchain.swapchain, &swapchainImageCount, nullptr);
	if (res != VK_SUCCESS || swapchainImageCount == 0) {
		throw std::exception("[ERROR] Vulkan: Couldn't get swap chain image count.");
	}

	swapchain.images.resize(swapchainImageCount);
	res = vkGetSwapchainImagesKHR(basics.device, swapchain.swapchain, &swapchainImageCount, &swapchain.images[0]);
	if (res != VK_SUCCESS || swapchainImageCount == 0) {
		throw std::exception("[ERROR] Vulkan: Couldn't get swap chain images");
	}

	swapchain.imageViews.resize(swapchain.images.size());
	swapchain.framebuffers.resize(swapchain.images.size());
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
			throw std::exception("Couldn't create swap chain image views.");
		}
	}
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

		auto res = vkCreateDescriptorPool(basics.device, &ci, nullptr, &descriptorPool);
		if (res != VK_SUCCESS) {
			throw std::exception("Vulkan: Couldn't create descriptor pool.");
		}
	}
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

	throw std::runtime_error("Failed to find suitable memory type.");
}

void Renderer::CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, size_t dstOffset, size_t size)
{
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = dstOffset;
	region.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &region);
}

void Renderer::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

VkCommandBuffer Renderer::CreateCommandBuffer(VkCommandBufferLevel level)
{
	VkCommandBuffer ret;
	VkCommandBufferAllocateInfo allocateInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		graphicsPool,
		level,
		1
};
	vkAllocateCommandBuffers(basics.device, &allocateInfo, &ret);
	return ret;
}

void Renderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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
}

#endif
