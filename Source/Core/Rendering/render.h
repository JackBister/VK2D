#pragma once
#include <cstddef>

#include "sprite.h"
#include "transform.h"

struct CameraComponent;
struct ResourceManager;
struct Shader;

struct Renderer {
	//If valid is false, the renderer is not in a complete state and should not be used.
	bool valid = false;

	virtual void Destroy() = 0;
	virtual void EndFrame() = 0;
	virtual bool Init(ResourceManager *, const char * title, int winX, int winY, int w, int h, uint32_t flags) = 0;
	virtual void RenderCamera(CameraComponent * const) = 0;

	virtual void AddSprite(Sprite * const) = 0;
	virtual void DeleteSprite(Sprite * const) = 0;
	
	virtual void AddCamera(CameraComponent * const) = 0;
	virtual void DeleteCamera(CameraComponent * const) = 0;

	virtual void AddShader(Shader * const) = 0;
	virtual void DeleteShader(Shader * const) = 0;
};

extern Renderer * Render_currentRenderer;

Renderer * GetOpenGLRenderer();
Renderer * GetVulkanRenderer();
