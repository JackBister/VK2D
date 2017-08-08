#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>

#include "Core/Queue.h"
#include "Core/Rendering/OpenGL/OpenGLRenderContext.h"
#include "Core/Rendering/RenderCommand.h"

class CameraComponent;
class Framebuffer;
class Image;
class Program;
class ResourceManager;
struct SDL_Window;
class Semaphore;
class Shader;
class Sprite;

class Renderer
{
public:
	Renderer(ResourceManager *, Queue<RenderCommand>::Reader&&, Semaphore * swapSem, char const * title, int winX, int winY, int w, int h, uint32_t flags) noexcept;
	~Renderer() noexcept;

	static std::unique_ptr<OpenGLRenderCommandContext> CreateCommandContext();

	void EndFrame(std::vector<std::unique_ptr<RenderCommandContext>>& commandBuffers) noexcept;
	uint64_t GetFrameTime() noexcept;

	//void AddBuffer(RenderCommand::AddBufferParams) noexcept;
	//void DeleteBuffer(BufferRendererData *) noexcept;

	/*
	void AddFramebuffer(Framebuffer * const) noexcept;
	void DeleteFramebuffer(Framebuffer * const) noexcept;

	void AddProgram(Program * const) noexcept;
	void DeleteProgram(Program * const) noexcept;

	void AddShader(Shader * const) noexcept;
	void DeleteShader(Shader * const) noexcept;
	*/

	void DrainQueue() noexcept;

	bool isAborting = false;
	int abortCode = 0;

	//TODO:
	static OpenGLFramebufferHandle Backbuffer;
	static OpenGLShaderModuleHandle ptVertexModule;
	static OpenGLShaderModuleHandle ptFragmentModule;

	//The window
	SDL_Window * window;

	//Total time between swapbuffers
	float frameTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	//Time spent on GPU in a frame
	GLuint timeQuery;

	bool swap;

private:
	ResourceManager * resourceManager;

	Queue<RenderCommand>::Reader renderQueue;

	//The aspect ratio of the viewport
	//TODO: Should this be attached to the camera?
	float aspectRatio;
	//The dimensions of the screen, in pixels
	glm::ivec2 dimensions;

	OpenGLImageHandle backbuffer;

	std::shared_ptr<Image> backBufferImage;
	std::shared_ptr<Framebuffer> backbufferRenderTarget;

	std::shared_ptr<Shader> ptvShader;
	std::shared_ptr<Shader> pfShader;

	//TODO:
	std::shared_ptr<Program> ptProgram;
	GLuint ptVAO;

	Semaphore * swapSem;

	uint32_t frameCount = 1;
	std::chrono::milliseconds totalSwapTime = std::chrono::milliseconds(0);
};
