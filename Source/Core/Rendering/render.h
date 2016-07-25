#pragma once
#include <cstddef>

#include "sprite.h"
#include "transform.h"

struct CameraComponent;

struct Renderer {
	//If valid is false, the renderer is not in a complete state and should not be used.
	bool valid = false;

	virtual void Destroy() = 0;
	virtual void EndFrame() = 0;
	virtual bool Init(const char * title, int winX, int winY, int w, int h, uint32_t flags) = 0;
	virtual void RenderCamera(CameraComponent * const) = 0;

	virtual void AddSprite(Sprite * const) = 0;
	virtual void DeleteSprite(Sprite * const) = 0;
	
	virtual void AddCamera(CameraComponent * const) = 0;
	virtual void DeleteCamera(CameraComponent * const) = 0;

	/*
	void (*Render_Destroy)();
	void (*Render_EndFrame)();
	bool (*Render_Init)(const char * title, int winX, int winY, int w, int h, uint32_t flags);
	void (*Render_RenderCamera)(CameraComponent *);

	void (*Render_AddSprite)(Sprite * const);
	void (*Render_DeleteSprite)(Sprite * const);

	void (*Render_AddCamera)(CameraComponent * const);
	void (*Render_DeleteCamera)(CameraComponent * const);
	*/
};

extern Renderer * Render_currentRenderer;

Renderer * GetOpenGLRenderer();
Renderer * GetVulkanRenderer();
