#include "render.h"

#include <cstdio>
#include <cstring>
#include <vector>

#include "glm/glm.hpp"
#include "SDL/SDL_syswm.h"
#include "SDL/SDL.h"
//TODO:
#if defined(_WIN32)
//I hate this:
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "vulkan/vulkan.h"

#include "Core/Rendering/Shader.h"
#include "Core/ResourceManager.h"

//The number of floats contained in one vertex
//vec3 pos, vec3 color, vec2 UV
#define VERTEX_SIZE 8

struct Model
{
	std::vector<float> vertices;
	std::vector<unsigned int> elements;
};

/*
struct VulkanImage
{
	VkImage image;
	VkImageView view;
	VkSampler sampler;
	VkDeviceMemory memory;
};
*/

struct VulkanSwapchain
{
	VkSwapchainKHR swapchain;
	VkFormat format;
	VkExtent2D extent;
};

struct VulkanRendererData
{

};

struct VulkanSprite
{
	Sprite * sprite;
	VulkanRendererData rendererData;
};

struct VulkanRenderer : Renderer
{
	void Destroy() override;
	void EndFrame() override;
	bool Init(ResourceManager *, const char * title, int winX, int winY, int w, int h, uint32_t flags) override;
	void RenderCamera(CameraComponent * const) override;

	void AddSprite(Sprite * const) override;
	void DeleteSprite(Sprite * const) override;

	void AddCamera(CameraComponent * const) override;
	void DeleteCamera(CameraComponent * const) override;

	void AddShader(Shader * const) override;
	void DeleteShader(Shader * const) override;

	//The device we use to render:
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	//Our Vulkan instance and surface:
	VkInstance instance;
	VkSurfaceKHR surface;
	//The queue we use to render and the queue we use to present and their indexes:
	uint32_t graphicsQueueIdx, presentQueueIdx;
	VkQueue graphicsQueue, presentQueue;
	//Command pools for graphics and present
	VkCommandPool graphicsPool, presentPool;
	//One command buffer for each frame in the swap chain
	//Each command buffer blits the result from the graphics queue to its image
	std::vector<VkCommandBuffer> presentCmdbufs;
	//A semaphore which is signaled when rendering has finished and we are ready to present:
	VkSemaphore imgAvailable, renderingFinished;
	//Swapchain
	VulkanSwapchain swapchain;
	VkRenderPass renderpass;

	//The aspect ratio of the viewport
	//TODO: Should this be attached to the camera?
	float aspectRatio;
	//All cameras the renderer knows about
	//TODO: Should Camera be separated from CameraComponent like Sprite from SpriteComponent?
	//TODO: Does Camera need a rendererData?
	std::vector<CameraComponent *> cameras;
	//The dimensions of the screen, in pixels
	glm::ivec2 dimensions;
	//All sprites
	std::vector<VulkanSprite> sprites;
	//The window
	SDL_Window * window;

	ResourceManager * resourceManager;

private:
	void Clear();
	bool Present();
};

//A quad with color 1.0, 1.0, 1.0
static Model plainQuad;
static unsigned int plainQuadElems[] = {
	0, 1, 2,
	2, 3, 0
};
static float plainQuadVerts[] = {
	//vec3 pos, vec3 color, vec2 texcoord
	-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
	1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
	-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
};

bool CheckVkDeviceExtensions(VkPhysicalDevice device, std::vector<const char *> extensions)
{
	//Get extension count
	uint32_t extensionCount = 0;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan device extension count.\n");
		return false;
	}

	//Get extension names
	auto availableExtensions = std::vector<VkExtensionProperties>(extensionCount);
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

bool CheckVkInstanceExtensions(std::vector<const char *> extensions)
{
	//Get extension count
	uint32_t extensionCount = 0;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
		printf("[ERROR] Unable to get Vulkan extension count.\n");
		return false;
	}

	//Get extension names
	auto availableExtensions = std::vector<VkExtensionProperties>(extensionCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
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

bool CreateVkCommandPool(const VkDevice& device, const uint32_t& queueIdx, VkCommandPool * const ret)
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

bool CreateVkSemaphore(const VkDevice& device, VkSemaphore * const ret)
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};
	if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, ret) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Unable to create image available semaphore.\n");
		return false;
	}
	return true;
}

Renderer * GetVulkanRenderer()
{
	return new VulkanRenderer();
	//return Renderer{ Render_Destroy, Render_EndFrame, Render_Init, Render_RenderCamera, Render_AddSprite, Render_DeleteSprite, Render_AddCamera, Render_DeleteCamera };
}

void VulkanRenderer::Clear()
{
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
		if (presentCmdbufs.size() > 0 && presentCmdbufs[0] != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(device, presentPool, static_cast<uint32_t>(presentCmdbufs.size()), &presentCmdbufs[0]);
			presentCmdbufs.clear();
		}
		if (presentPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device, presentPool, nullptr);
			presentPool = VK_NULL_HANDLE;
		}
	}
}

bool VulkanRenderer::Present()
{
	uint32_t imgIdx;
	VkResult result = vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, imgAvailable, VK_NULL_HANDLE, &imgIdx);
	switch (result) {
	case VK_SUCCESS:
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		printf("[UNIMPLEMENTED] Vulkan: Window size changed.\n");
		return false;
	default:
		printf("[ERROR] Vulkan: Error occured during image swap chain acquisition.\n");
		return false;
	}
	
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		1,
		&imgAvailable,
		&waitDstStageMask,
		1,
		&presentCmdbufs[imgIdx],
		1,
		&renderingFinished
	};
	if (vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		return false;
	}

	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&renderingFinished,
		1,
		&swapchain.swapchain,
		&imgIdx,
		nullptr
	};
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	switch (result) {
	case VK_SUCCESS:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
	case VK_SUBOPTIMAL_KHR:
		printf("[UNIMPLEMENTED] Vulkan: Window size changed (after queue present).\n");
		return false;
	default:
		printf("[ERROR] Vulkan: Error occurred during image presentation.\n");
		return false;
	}
	return true;
}

void VulkanRenderer::Destroy()
{
	//TODO: Probably going to be missing some stuff at times here
	Clear();
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);

		if (imgAvailable != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, imgAvailable, nullptr);
		}
		if (renderingFinished != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, renderingFinished, nullptr);
		}
		if (swapchain.swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
		}
		vkDestroyDevice(device, nullptr);
	}

	if (instance != VK_NULL_HANDLE) {
		if (surface != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
	}
	SDL_DestroyWindow(window);
	valid = false;
}

void VulkanRenderer::EndFrame()
{
	for (auto& camera : cameras) {
		RenderCamera(camera);
	}

	Present();
	//TODO:
	//SDL_GL_SwapWindow(window);
}

#undef _DEBUG

#if defined(_DEBUG)
VkBool32 VKAPI_PTR dbgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char * pLayerPrefix, const char * pMessage, void * pUserData)
{
	printf("[%s] %s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}
#endif

/*
	Largely taken from: https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
	Most stuff is inlined for the time being since it's only used in one place, so the function looks huge but is straightforward to read.
	I broke it up using lambdas to make it explicit which parts of the code affect which member variables.
	TODO: WTF: Putting some parts in blocks/lambdas causes vkGetPhysicalDeviceSurfaceCapabilitiesKHR to fail.
*/
bool VulkanRenderer::Init(ResourceManager * resMan,  const char * title, const int winX, const int winY, const int w, const int h, const uint32_t flags)
{
	aspectRatio = static_cast<float>(w) / static_cast<float>(h);
	dimensions = glm::ivec2(w, h);
	window = SDL_CreateWindow(title, winX, winY, w, h, flags);
	resourceManager = resMan;

	//The extensions the renderer needs
	std::vector<const char *> instanceExtensions = {
#if defined(_DEBUG)
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME,
		//TODO: Other WMs
		#if defined(VK_USE_PLATFORM_WIN32_KHR)
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME
		#endif
	};

	if (!CheckVkInstanceExtensions(instanceExtensions)) {
		printf("[ERROR] Vulkan: Required instance extensions not supported.\n");
		return false;
	}

	std::vector<const char *> instanceLayers = {
#if defined(_DEBUG)
		"VK_LAYER_LUNARG_standard_validation",
#endif
#if defined(VULKAN_API_DUMP)
		"VK_LAYER_LUNARG_api_dump"
#endif
	};

	/*
		Create Vulkan instance
	*/
	printf("inst pre %d\n", instance);
	//{
		const VkInstanceCreateFlags vulkanFlags = 0;
		const VkInstanceCreateInfo vulkanInfo = {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			nullptr,
			vulkanFlags,
			nullptr,
			static_cast<uint32_t>(instanceLayers.size()),
			instanceLayers.size() > 0 ? &instanceLayers[0] : nullptr,
			static_cast<uint32_t>(instanceExtensions.size()),
			instanceExtensions.size() > 0 ? &instanceExtensions[0] : nullptr,
		};

		printf("Creating instance %p \n", &vulkanInfo);
		if (vkCreateInstance(&vulkanInfo, nullptr, &instance) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Failed to create instance\n");
			return false;
		}
		printf("inst in block %d\n", instance);
		printf("&inst in block %p\n", &instance);
	//}
	printf("inst post %d\n", instance);
	printf("&inst post block %p\n", &instance);
#if defined(_DEBUG)
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
	PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

	VkDebugReportCallbackCreateInfoEXT dbgInfo = {
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		nullptr,
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		dbgCallback,
		nullptr
	};

	VkDebugReportCallbackEXT test;
	if (vkCreateDebugReportCallbackEXT(instance, &dbgInfo, nullptr, &test) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Failed to create debug report callback.\n");
		return false;
	}
#endif

	SDL_SysWMinfo wmInfo;
	SDL_GetWindowWMInfo(window, &wmInfo);

	/*
		Platform specific surface creation
		TODO: Other WMs
		TODO: WTF: If this part of the code or the instance creation code is put in a block or lambda, vkGetPhysicalDeviceSurfaceCapabilitiesKHR will return VK_ERROR_SURFACE_LOST
	*/
	//bool surfaceResult = [] (const VkInstance& instance, const SDL_SysWMinfo& wmInfo, VkSurfaceKHR * const ret) -> bool {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		//I hate Windows APIs
		//TODO: I suspect this line of code is the cause of a lot of pain:
	HINSTANCE win32Instance = (HINSTANCE)GetWindowLongPtr(wmInfo.info.win.window, GWLP_HINSTANCE);
	VkWin32SurfaceCreateFlagsKHR win32SurfaceFlags = 0;
	VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		nullptr,
		win32SurfaceFlags,
		win32Instance,
		wmInfo.info.win.window
	};
	printf("[TRACE] Vulkan: Creating win32 surface.\n");
	if (vkCreateWin32SurfaceKHR(instance, &win32SurfaceInfo, nullptr, &surface) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Couldn't create Win32 surface.\n");
		return false;
	}
#endif
	/*	} (instance, wmInfo, &surface);

		if (!surfaceResult) {
			return false;
		}
	*/
	/*
		Chose physical device to use.
		TODO: Device chosen is simply the first discrete one, otherwise falls back to integrated, then CPU.
		TODO: In the future, could/should use multiple physical devices and take more care about what discrete GPU is used.
	*/
	bool physDeviceResult = [](const VkInstance& instance, VkPhysicalDevice * const ret) -> bool {
		uint32_t deviceCount = 0;
		if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
			printf("[ERROR] Vulkan: Couldn't enumerate physical devices.\n");
			return false;
		}
		auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
		if (vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevices[0]) != VK_SUCCESS || deviceCount == 0) {
			printf("[ERROR] Vulkan: Couldn't enumerate physical devices.\n");
			return false;
		}

		VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceType chosenType;
		for (auto& pd : physicalDevices) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(pd, &props);
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				printf("[TRACE] Vulkan: Discrete GPU detected.\n");
				chosenDevice = pd;
				chosenType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				break;
			}
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
				printf("[TRACE] Vulkan: iGPU detected.\n");
				chosenDevice = pd;
				chosenType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
			} else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU && chosenType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
				printf("[TRACE] Vulkan: CPU detected.\n");
				chosenDevice = pd;
				chosenType = VK_PHYSICAL_DEVICE_TYPE_CPU;
			}
		}
		if (chosenDevice == VK_NULL_HANDLE) {
			printf("[TRACE] Vulkan: chosenDevice is VK_NULL_HANDLE, defaulting...\n");
			chosenDevice = physicalDevices[0];
		}
		*ret = chosenDevice;
		return true;
	} (instance, &physicalDevice);

	if (!physDeviceResult) {
		return false;
	}

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (!CheckVkDeviceExtensions(physicalDevice, deviceExtensions)) {
		printf("[ERROR] Vulkan: Required device extensions not supported.\n");
		return false;
	}

	/*
		Create device and queues
	*/
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	if (queueFamilyCount == 0) {
		printf("[ERROR] Vulkan: No queue families found.\n");
		return false;
	}

	auto queueFamilyProperties = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
	auto queuePresentSupport = std::vector<VkBool32>(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, &queueFamilyProperties[0]);

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &queuePresentSupport[i]) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Couldn't get physical device surface support.\n");
			return false;
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
		"VK_LAYER_LUNARG_standard_validation"
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

	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Couldn't create device.\n");
		return false;
	}

	vkGetDeviceQueue(device, graphicsQueueIdx, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentQueueIdx, 0, &presentQueue);

	/*
		Create semaphore indicating that we have acquired an image from the swapchain
	*/
	if(!CreateVkSemaphore(device, &imgAvailable)) {
		return false;
	}

	/*
		Create semaphore indicating that rendering has finished and presenting may begin
	*/
	if (!CreateVkSemaphore(device, &renderingFinished)) {
		return false;
	}

	/*
		Get required info and create swap chain
	*/
	//{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Unable to get device surface capabilities.\n");
			return false;
		}

		uint32_t formatsCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr) != VK_SUCCESS || formatsCount == 0) {
			printf("[ERROR] Vulkan: Unable to get device surface formats.\n");
			return false;
		}

		auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(formatsCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, &surfaceFormats[0]) != VK_SUCCESS || formatsCount == 0) {
			printf("[ERROR] Vulkan: Unable to get device surface formats.\n");
			return false;
		}

		uint32_t presentModesCount;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr) != VK_SUCCESS || presentModesCount == 0) {
			printf("[ERROR] Vulkan: Unable to get device present modes.\n");
			return false;
		}

		auto presentModes = std::vector<VkPresentModeKHR>(presentModesCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, &presentModes[0]) != VK_SUCCESS || presentModesCount == 0) {
			printf("[ERROR] Vulkan: Unable to get device present modes.\n");
			return false;
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
			printf("[ERROR] Vulkan: VK_IMAGE_USAGE_TRANSFER_DST_BIT not supported by surface.\n");
			return false;
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
			printf("[ERROR] Vulkan: Required present modes not supported.\n");
			return false;
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
			VK_TRUE,
			VK_NULL_HANDLE
		};

		if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.swapchain) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Can't create swap chain.\n");
			return false;
		}
		swapchain.extent = desiredExtent;
		swapchain.format = desiredFormat.format;

//	}
	/*
		Create command pools
	*/
	if(!CreateVkCommandPool(device, graphicsQueueIdx, &graphicsPool)) {
		return false;
	}

	if (!CreateVkCommandPool(device, presentQueueIdx, &presentPool)) {
		return false;
	}

	/*
		Create present command buffers
	*/
	const bool presentCommandBuffersResult = [](const VkDevice& device, const VkSwapchainKHR& swapchain, const VkCommandPool& pool, std::vector<VkCommandBuffer> * const pCmdBufs) -> bool {
		uint32_t swapchainImageCount = 0;
		if (vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS || swapchainImageCount == 0) {
			printf("[ERROR] Vulkan: Couldn't get swap chain image count.\n");
			return false;
		}
		pCmdBufs->resize(swapchainImageCount);

		VkCommandBufferAllocateInfo presentCmdbufInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			pool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			swapchainImageCount
		};
		if (vkAllocateCommandBuffers(device, &presentCmdbufInfo, pCmdBufs->data()) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Couldn't allocate present command buffers.\n");
			return false;
		}
		return true;
	} (device, swapchain.swapchain, presentPool, &presentCmdbufs);

	if (!presentCommandBuffersResult) {
		return false;
	}

	/*
		Record present command buffers
		TODO: When I made this a lambda the vkGetPhysicalDeviceSurfaceCapabilitiesKHR call hundreds of lines above this point in the code started failing... What the fuck is going on?
	*/
	{
		uint32_t swapchainImageCount = 0;
		if (vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS || swapchainImageCount == 0) {
			printf("[ERROR] Vulkan: Couldn't get swap chain image count.\n");
			return false;
		}
		auto swapchainImages = std::vector<VkImage>(swapchainImageCount);
		if (vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, &swapchainImages[0]) != VK_SUCCESS) {
			printf("[ERROR] Vulkan: Couldn't get swap chain images.\n");
			return false;
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
			//Memory barrier that moves from VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
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
			//Memory barrier that moves from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR
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
			vkBeginCommandBuffer(presentCmdbufs[i], &presentBufBeginInfo);
			vkCmdPipelineBarrier(presentCmdbufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToClear);
			//TODO: Replace with blit from graphics queue backbuffer?
			vkCmdClearColorImage(presentCmdbufs[i], swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imgSubresourceRange);
			vkCmdPipelineBarrier(presentCmdbufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClearToPresent);
			if (vkEndCommandBuffer(presentCmdbufs[i]) != VK_SUCCESS) {
				printf("[ERROR] Vulkan: Unable to record present command buffer.\n");
				return false;
			}
		}
	}

	VkAttachmentDescription attachmentDescriptions[] = {
		{
			0,
			swapchain.format,
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
	/*
	if (vkCreateRenderPass(device, &renderpassCreateInfo, nullptr, &renderpass) != VK_SUCCESS) {
		printf("[ERROR] Vulkan: Couldn't create render pass. %d\n", vkCreateRenderPass(device, &renderpassCreateInfo, nullptr, &renderpass));
		return false;
	}
	*/
	valid = true;
	return true;
}

void VulkanRenderer::RenderCamera(CameraComponent * camera)
{

}

void VulkanRenderer::AddSprite(Sprite * const sprite)
{
	VulkanSprite s;
	s.sprite = sprite;
	sprite->rendererData = &s.rendererData;

	sprites.push_back(s);
}

void VulkanRenderer::DeleteSprite(Sprite * const sprite)
{
	for (auto it = sprites.begin(); it != sprites.end(); ++it) {
		if (it->sprite == sprite) {
			sprites.erase(it);
		}
	}
}

void VulkanRenderer::AddCamera(CameraComponent * const camera)
{
	cameras.push_back(camera);
}

void VulkanRenderer::DeleteCamera(CameraComponent * const camera)
{
	for (auto it = cameras.begin(); it != cameras.end(); ++it) {
		if (*it == camera) {
			cameras.erase(it);
		}
	}
}

void VulkanRenderer::AddShader(Shader * const)
{
}

void VulkanRenderer::DeleteShader(Shader * const)
{
}
