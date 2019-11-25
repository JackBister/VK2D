#ifdef USE_OGL_RENDERER
#include "Core/Rendering/Backend/OpenGL/OpenGLRenderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stb_image.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLCommandBuffer.h"
#include "Core/Rendering/Backend/OpenGL/OpenGLResourceContext.h"
#include "Core/Util/SetThreadName.h"

static const auto logger = Logger::Create("OpenGLRenderer");

Renderer::~Renderer()
{
    SDL_DestroyWindow(window);
}

Renderer::Renderer(char const * title, int const winX, int const winY, uint32_t const flags, RendererConfig config)
    : config(config), renderQueueRead(renderQueue.GetReader()), renderQueueWrite(renderQueue.GetWriter())
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    window = SDL_CreateWindow(
        title, winX, winY, config.windowResolution.x, config.windowResolution.y, flags | SDL_WINDOW_OPENGL);
    auto ctx = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        logger->Errorf("GLEW Error: %s", glewGetErrorString(err));
    }
    logger->Infof("Program start glGetError(Expected 1280): %d", glGetError());

    stbi_set_flip_vertically_on_load(true);

    glReadBuffer(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_GL_MakeCurrent(window, nullptr);
    renderThread = std::thread(&Renderer::RenderThread, this, ctx);
    SetThreadName(renderThread.get_id(), "OpenGL RenderThread");
}

void Renderer::CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderQueueWrite.Push(RenderCommand(RenderCommand::CreateResourceParams(fun)));
}

void Renderer::ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                                    std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
{
    renderQueueWrite.Push(
        RenderCommand(RenderCommand::ExecuteCommandContextParams(ctx, waitSem, signalSem, signalFence)));
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
    return std::vector<FramebufferHandle *>{&backbuffer};
}

Format Renderer::GetBackbufferFormat() const
{
    return Format::RGBA8;
}

glm::ivec2 Renderer::GetResolution() const
{
    return config.windowResolution;
}

uint32_t Renderer::GetSwapCount() const
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

void Renderer::DrainQueue()
{
    using namespace std::literals::chrono_literals;
    RenderCommand command = renderQueueRead.Wait();
    switch (command.params.index()) {
    case RenderCommand::Type::ABORT:
        isAborting = true;
        abortCode = std::get<RenderCommand::AbortParams>(command.params).errorCode;
        break;
    case RenderCommand::Type::CREATE_RESOURCE: {
        auto fun = std::get<RenderCommand::CreateResourceParams>(command.params).fun;
        OpenGLResourceContext ctx;
        glFlush();
        glFinish();
        fun(ctx);
        glFlush();
        glFinish();
        break;
    }
    case RenderCommand::Type::EXECUTE_COMMAND_CONTEXT: {
        auto params = std::get<RenderCommand::ExecuteCommandContextParams>(command.params);
        params.ctx->Execute(this, params.waitSem, params.signalSem, params.signalFence);
        break;
    }
    case RenderCommand::Type::SWAP_WINDOW: {
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
        // With Aero enabled in Windows 7, this doesn't busy wait.
        // With Aero disabled, it does, causing 100% use on the GPU thread. Crazy.
        SDL_GL_SwapWindow(window);
        // TODO: These are both necessary to reduce CPU usage
        // My understanding is that without these SwapWindow returns instantly and spins up its own thread(?) that
        // spinlocks, borking our totalSwapTime.
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
        logger->Warnf("Unimplemented render command: %zu", command.params.index());
    }
    GLenum err = glGetError();
    if (err) {
        logger->Errorf("RenderQueue pop error %u. RenderCommand %zu.", err, command.params.index());
    }
}
#endif
