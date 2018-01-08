#ifndef USE_VULKAN_RENDERER
#include "Core/Rendering/OpenGL/OpenGLRenderer.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <stb_image.h>

#include "Core/Rendering/OpenGL/OpenGLRenderContext.h"

Renderer::~Renderer() noexcept
{
	SDL_DestroyWindow(window);
}

Renderer::Renderer(char const * title, int const winX, int const winY, int const w, int const h, uint32_t const flags) noexcept
	: rendQueueRead(renderQueue.GetReader()), renderQueueWrite(renderQueue.GetWriter())
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	aspectRatio = static_cast<float>(w) / static_cast<float>(h);
	dimensions = glm::ivec2(w, h);
	window = SDL_CreateWindow(title, winX, winY, w, h, flags | SDL_WINDOW_OPENGL);
	auto ctx = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
	}
	printf("Program start glGetError(Expected 1280): %d\n", glGetError());

	stbi_set_flip_vertically_on_load(true);

	glReadBuffer(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_GL_MakeCurrent(window, nullptr);
	renderThread = std::thread(&Renderer::RenderThread, this, ctx);
}

void Renderer::CreateResources(std::function<void(ResourceCreationContext&)> fun)
{
	renderQueueWrite.Push(RenderCommand(RenderCommand::CreateResourceParams(fun)));
}

void Renderer::ExecuteCommandContext(RenderCommandContext * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
	renderQueueWrite.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(ctx, waitSem, signalSem, signalFence)));
}

void Renderer::SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem)
{
	renderQueueWrite.Push(RenderCommand(RenderCommand::SwapWindowParams(imageIndex, waitSem)));
}

uint32_t Renderer::AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) 
{
	if (signalSem != nullptr) {
		((OpenGLSemaphoreHandle *)signalSem)->sem.Signal();
	}
	if (signalFence != nullptr) {
		((OpenGLFenceHandle *)signalFence)->sem.Signal();
	}
	return 0;
}

std::vector<FramebufferHandle *> Renderer::CreateBackbuffers(RenderPassHandle * renderPass) 
{
	return std::vector<FramebufferHandle *>{
		&backbuffer
	};
}

Format Renderer::GetBackbufferFormat() 
{
	return Format::RGBA8;
}

uint32_t Renderer::GetSwapCount() 
{
	return 1;
}

void Renderer::RenderThread(SDL_GLContext ctx)
{
	SDL_GL_MakeCurrent(window, ctx);
	while (!isAborting) {
		DrainQueue();
	}
}

void Renderer::DrainQueue() noexcept
{
	using namespace std::literals::chrono_literals;
	RenderCommand command = rendQueueRead.Wait();
	switch (command.params.index()) {
	case RenderCommand::Type::ABORT:
		isAborting = true;
		abortCode = std::get<RenderCommand::AbortParams>(command.params).errorCode;
		break;
	case RenderCommand::Type::CREATE_RESOURCE:
	{
		auto fun = std::get<RenderCommand::CreateResourceParams>(command.params).fun;
		OpenGLResourceContext ctx;
		fun(ctx);
		break;
	}
	case RenderCommand::Type::EXECUTE_COMMAND_CONTEXT:
	{
		auto params = std::get<RenderCommand::ExecuteCommandContextParams>(command.params);
		params.ctx->Execute(this, params.waitSem, params.signalSem, params.signalFence);
		break;
	}
	case RenderCommand::Type::SWAP_WINDOW:
	{
		auto params = std::get<RenderCommand::SwapWindowParams>(command.params);
		auto nativeWait = (OpenGLSemaphoreHandle *)params.waitSem;
		auto preSwap = std::chrono::high_resolution_clock::now();
		auto res = preSwap - lastTime;
		auto avgSwapTime = totalSwapTime / frameCount;
		if (res + avgSwapTime < 8.333334ms) {
			std::this_thread::sleep_for((totalSwapTime / frameCount) - 0.5ms);
		}
		if (nativeWait != nullptr) {
			nativeWait->sem.Wait();
		}
		//With Aero enabled in Windows 7, this doesn't busy wait.
		//With Aero disabled, it does, causing 100% use on the GPU thread. Crazy.
		SDL_GL_SwapWindow(window);
		//TODO: These are both necessary to reduce CPU usage
		//My understanding is that without these SwapWindow returns instantly and spins up its own thread(?) that spinlocks, borking our totalSwapTime. 
		glFlush();
		glFinish();
		auto postSwap = std::chrono::high_resolution_clock::now();
		frameTime = static_cast<float>((postSwap - lastTime).count());
		lastTime = postSwap;

		frameCount++;
		totalSwapTime += std::chrono::duration_cast<std::chrono::milliseconds>(postSwap - preSwap);
		break;
	}
	default:
		printf("[WARNING] Unimplemented render command: %zu\n", command.params.index());
	}
	GLenum err = glGetError();
	if (err) {
		printf("RenderQueue pop error %u. RenderCommand %zu.\n", err, command.params.index());
	}
}
#endif
