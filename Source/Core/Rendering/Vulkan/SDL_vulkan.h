#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <optional>

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>


std::optional<VkSurfaceKHR> SDL_VK_CreateSurface(SDL_Window * window, VkInstance instance) {
	VkSurfaceKHR surface;
	SDL_SysWMinfo wmInfo;
	SDL_GetWindowWMInfo(window, &wmInfo);

#ifdef _WIN32 
	HINSTANCE win32Instance = (HINSTANCE)GetWindowLongPtr(wmInfo.info.win.window, GWLP_HINSTANCE);
	VkWin32SurfaceCreateFlagsKHR win32SurfaceFlags = 0;
	VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		nullptr,
		win32SurfaceFlags,
		win32Instance,
		wmInfo.info.win.window
	};
	if (vkCreateWin32SurfaceKHR(instance, &win32SurfaceInfo, nullptr, &surface) != VK_SUCCESS) {
		printf("[ERROR] %s:%d Vulkan: Couldn't create Win32 surface.\n", __FILE__, __LINE__);
		return {};
	}
#endif

	return surface;
}
