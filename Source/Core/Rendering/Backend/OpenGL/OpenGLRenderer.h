#pragma once
#ifdef USE_OGL_RENDERER
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>

#include "Core/Queue.h"
#include "Core/Rendering/Backend/Abstract/AbstractRenderer.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLContextStructs.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLResourceContext.h"
#include "Core/Rendering/Backend/OpenGL/RenderCommand.h"

class Renderer : public IRenderer
{
public:
    Renderer(char const * title, int winX, int winY, uint32_t flags, RendererConfig config);
    ~Renderer();

    uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
    std::vector<FramebufferHandle *> CreateBackbuffers(RenderPassHandle * renderPass) final override;
    Format GetBackbufferFormat() const final override;
    glm::ivec2 GetResolution() const final override;
    uint32_t GetSwapCount() const final override;

    void CreateResources(std::function<void(ResourceCreationContext &)> fun) final override;
    void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                              std::vector<SemaphoreHandle *> signalSem,
                              FenceHandle * signalFence = nullptr) final override;
    void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) final override;

    void RecreateSwapchain() final override;

	RendererConfig GetConfig() final override;
    void UpdateConfig(RendererConfig) final override;

    int abortCode = 0;

private:
    void DrainQueue();
    void RenderThread(SDL_GLContext ctx);
    void UpdatePresentMode();

    bool isAborting = false;
    uint32_t frameCount = 1;
    float frameTime;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::thread renderThread;

    Queue<RenderCommand> renderQueue;
    Queue<RenderCommand>::Reader renderQueueRead;
    Queue<RenderCommand>::Writer renderQueueWrite;

    RendererConfig config;

    std::chrono::milliseconds totalSwapTime = std::chrono::milliseconds(0);

    OpenGLFramebufferHandle backbuffer;

    SDL_Window * window;
};
#endif
