#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>

#include "Core/Queue.h"
#include "Core/Rendering/OpenGL/OpenGLRendererData.h"
#include "Core/Rendering/RenderCommand.h"

struct CameraComponent;
struct Framebuffer;
struct Image;
struct Program;
struct ResourceManager;
struct SDL_Window;
struct Shader;
struct Sprite;

struct Renderer
{
	void EndFrame(std::vector<SubmittedCamera>& cameras, std::vector<SubmittedSprite>& sprites) noexcept;
	Renderer(ResourceManager *, Queue<RenderCommand>::Reader&&, Queue<ViewDef *>::Writer&&, const char * title, int winX, int winY, int w, int h, uint32_t flags) noexcept;
	~Renderer() noexcept;
	uint64_t GetFrameTime() noexcept;

	void AddFramebuffer(Framebuffer * const) noexcept;
	void DeleteFramebuffer(Framebuffer * const) noexcept;

	void AddImage(Image * const) noexcept;
	void DeleteImage(Image * const) noexcept;

	void AddProgram(Program * const) noexcept;
	void DeleteProgram(Program * const) noexcept;

	void AddShader(Shader * const) noexcept;
	void DeleteShader(Shader * const) noexcept;

	void DrainQueue() noexcept;

	bool isAborting = false;
	int abortCode = 0;

private:
	ResourceManager * resourceManager;

	Queue<RenderCommand>::Reader renderQueue;
	Queue<ViewDef *>::Writer viewDefQueue;

	//The aspect ratio of the viewport
	//TODO: Should this be attached to the camera?
	float aspectRatio;
	//The dimensions of the screen, in pixels
	glm::ivec2 dimensions;
	//The window
	SDL_Window * window;

	ImageRendererData backbufferTexture;

	std::shared_ptr<Image> backBufferImage;
	std::shared_ptr<Framebuffer> backbufferRenderTarget;

	std::shared_ptr<Shader> ptvShader;
	std::shared_ptr<Shader> pfShader;

	//TODO:
	std::shared_ptr<Program> ptProgram;
	GLuint ptVAO;

	//Total time between swapbuffers
	float frameTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	//Time spent on GPU in a frame
	GLuint timeQuery;
};