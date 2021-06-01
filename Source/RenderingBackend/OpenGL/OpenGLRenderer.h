#pragma once
#ifdef USE_OGL_RENDERER
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <gl/glew.h>
#include <glm/glm.hpp>

#include "../Abstract/AbstractRenderer.h"
#include "OpenGLContextStructs.h"
#include "OpenGLResourceContext.h"
#include "RenderCommand.h"
#include "Util/Queue.h"

class Renderer : public IRenderer
{
public:
    Renderer(char const * title, int winX, int winY, uint32_t flags, RendererConfig config, int numThreads);
    ~Renderer();

    uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) final override;
    std::vector<ImageViewHandle *> GetBackbuffers() final override;
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

    RendererProperties const & GetProperties() final override;

    int abortCode = 0;

private:
    void DrainQueue();
    void RenderThread(SDL_GLContext ctx);
    void UpdatePresentMode();

    void CmdAbort(RenderCommand const &);
    void CmdCreateResources(RenderCommand const &);
    void CmdExecute(RenderCommand const &);
    void CmdSwapWindow(RenderCommand const &);

    bool isAborting = false;
    uint32_t frameCount = 1;
    float frameTime;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::thread renderThread;

    Queue<RenderCommand> renderQueue;
    Queue<RenderCommand>::Reader renderQueueRead;
    Queue<RenderCommand>::Writer renderQueueWrite;

    RendererConfig config;
    RendererProperties properties;

    std::chrono::milliseconds totalSwapTime = std::chrono::milliseconds(0);

    bool backbufferIsStale = false;
    GLuint backbufferFramebuffer;
    OpenGLImageHandle backbufferImage;
    OpenGLImageViewHandle backbufferView;

    SDL_Window * window;
};
#endif
