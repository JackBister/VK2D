#pragma once

/*
        "Static inheritance":
        In a perfect world all renderers would be included in one build and could be switched at runtime,
        but I can't seem to accomplish this without some form of overhead in the form of pointer indirection
        so this is how it's going to be done for now.
        DOOM(2016) does something like this, since it contains a separate executable for the Vulkan renderer.
*/
#ifndef USE_OGL_RENDERER
#include "Vulkan/VulkanRenderer.h"
#else
#include "OpenGL/OpenGLRenderer.h"
#endif
