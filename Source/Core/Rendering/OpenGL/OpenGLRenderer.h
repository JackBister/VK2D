#pragma once
#ifndef USE_VULKAN_RENDERER
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>

#include "Core/Queue.h"
#include "Core/Rendering/AbstractRenderer.h"
#include "Core/Rendering/OpenGL/OpenGLRenderContext.h"
#include "Core/Rendering/RenderCommand.h"

class Renderer : public IRenderer
{
public:
	Renderer(/*ResourceManager *, Queue<RenderCommand>::Reader&&,*/ char const * title, int winX, int winY, int w, int h, uint32_t flags) noexcept;
	~Renderer() noexcept;
	
	uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
	std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) final override;
	Format GetBackbufferFormat() final override;
	uint32_t GetSwapCount() final override;

	void CreateResources(std::function<void(ResourceCreationContext&)> fun) final override;
	void ExecuteCommandContext(RenderCommandContext * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence = nullptr) final override;
	void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) final override;

	int abortCode = 0;

private:
	void DrainQueue() noexcept;
	void RenderThread(SDL_GLContext ctx);

	bool isAborting = false;
	uint32_t frameCount = 1;
	float frameTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	std::thread renderThread;

	Queue<RenderCommand> renderQueue;
	Queue<RenderCommand>::Reader rendQueueRead;
	Queue<RenderCommand>::Writer renderQueueWrite;

	//The aspect ratio of the window
	float aspectRatio;
	//The dimensions of the screen, in pixels
	glm::ivec2 dimensions;

	std::chrono::milliseconds totalSwapTime = std::chrono::milliseconds(0);

	OpenGLFramebufferHandle backbuffer;

	SDL_Window * window;
};
#endif
