#pragma once
#include <cstdint>
#if 0
struct CameraComponent;
struct Framebuffer;
struct Image;
struct ResourceManager;
struct Shader;
struct Sprite;

struct Renderer {
	//If valid is false, the renderer is not in a complete state and should not be used.
	bool valid = false;

	virtual void Destroy() = 0;
	virtual void EndFrame() = 0;
	//Return elapsed GPU time on last frame if available, 0 if not.
	virtual uint64_t GetFrameTime() = 0;
	virtual bool Init(ResourceManager *, char const * title, int winX, int winY, int w, int h, uint32_t flags) = 0;
	virtual void RenderCamera(CameraComponent * const) = 0;

	virtual void AddSprite(Sprite * const) = 0;
	virtual void DeleteSprite(Sprite * const) = 0;
	
	virtual void AddCamera(CameraComponent * const) = 0;
	virtual void DeleteCamera(CameraComponent * const) = 0;

	virtual void AddFramebuffer(Framebuffer * const) = 0;
	virtual void DeleteFramebuffer(Framebuffer * const) = 0;

	virtual void AddImage(Image * const) = 0;
	virtual void DeleteImage(Image * const) = 0;

	virtual void AddShader(Shader * const) = 0;
	virtual void DeleteShader(Shader * const) = 0;
};

extern Renderer * Render_currentRenderer;

Renderer * GetOpenGLRenderer();
Renderer * GetVulkanRenderer();
#endif
