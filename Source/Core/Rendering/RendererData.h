#pragma once

//See Renderer.h

#ifdef USE_VULKAN_RENDERER
#include "Core/Rendering/Vulkan/VulkanRendererData.h"
#else
#include "Core/Rendering/OpenGL/OpenGLRendererData.h"
#endif
