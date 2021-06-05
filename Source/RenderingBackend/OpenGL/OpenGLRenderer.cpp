#ifdef USE_OGL_RENDERER
#include "OpenGLRenderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <optick/optick.h>
#include <stb_image.h>

#include "Logging/Logger.h"
#include "OpenGLCommandBuffer.h"
#include "OpenGLResourceContext.h"
#include "Util/SetThreadName.h"

static const auto logger = Logger::Create("OpenGLRenderer");
static const auto openGlLogger = Logger::Create("OpenGL");

#ifdef _DEBUG
static void DbgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const * message,
                        void const * userParam)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        openGllogger.Error("{}", message);
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
        openGllogger.Warn("{}", message);
    } else {
        openGllogger.Info("{}", message);
    }
}
#endif

Renderer::~Renderer()
{
    SDL_DestroyWindow(window);
}

Renderer::Renderer(char const * title, int const winX, int const winY, uint32_t const flags, RendererConfig config,
                   int numThreads)
    : config(config), renderQueueRead(renderQueue.GetReader()), renderQueueWrite(renderQueue.GetWriter())
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifdef _DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
    window = SDL_CreateWindow(
        title, winX, winY, config.windowResolution.x, config.windowResolution.y, flags | SDL_WINDOW_OPENGL);
    auto ctx = SDL_GL_CreateContext(window);
    UpdatePresentMode();

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        logger.Error("GLEW Error: {}", glewGetErrorString(err));
    }
    logger.Info("Program start glGetError(Expected 1280): {}", glGetError());

    stbi_set_flip_vertically_on_load(true);

    glReadBuffer(GL_BACK);
    glGenFramebuffers(1, &backbufferFramebuffer);
    RecreateSwapchain();

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DbgCallback, nullptr);
#endif

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
    OPTICK_EVENT();
    if (signalSem != nullptr) {
        ((OpenGLSemaphoreHandle *)signalSem)->sem.Signal();
    }
    if (signalFence != nullptr) {
        ((OpenGLFenceHandle *)signalFence)->sem.Signal();
    }
    if (backbufferIsStale) {
        return UINT32_MAX;
    }
    return 0;
}

std::vector<ImageViewHandle *> Renderer::GetBackbuffers()
{
    return std::vector<ImageViewHandle *>{&backbufferView};
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
    OPTICK_THREAD("OpenGL RenderThread");
    SDL_GL_MakeCurrent(window, ctx);
    while (!isAborting) {
        DrainQueue();
    }
}

void Renderer::UpdatePresentMode()
{
    if (config.presentMode == PresentMode::IMMEDIATE) {
        SDL_GL_SetSwapInterval(0);
    } else if (config.presentMode == PresentMode::FIFO) {
        SDL_GL_SetSwapInterval(1);
    } else if (config.presentMode == PresentMode::MAILBOX) {
        logger.Warn("Mailbox present mode is not supported on OpenGL");
    }
}

void Renderer::DrainQueue()
{
    RenderCommand command = renderQueueRead.Wait();
    switch (command.params.index()) {
    case RenderCommand::Type::ABORT:
        CmdAbort(command);
        break;
    case RenderCommand::Type::CREATE_RESOURCE: {
        CmdCreateResources(command);
        break;
    }
    case RenderCommand::Type::EXECUTE_COMMAND_CONTEXT: {
        CmdExecute(command);
        break;
    }
    case RenderCommand::Type::SWAP_WINDOW: {
        CmdSwapWindow(command);
        break;
    }
    default:
        logger.Warn("Unimplemented render command: {}", command.params.index());
    }
    GLenum err = glGetError();
    if (err) {
        logger.Error("RenderQueue pop error {}. RenderCommand {}.", err, command.params.index());
    }
}

void Renderer::RecreateSwapchain()
{
    backbufferIsStale = false;

    if (backbufferImage.nativeHandle != 0) {
        glDeleteTextures(1, &backbufferImage.nativeHandle);
    }

    glGenTextures(1, &backbufferImage.nativeHandle);
    glBindTexture(GL_TEXTURE_2D, backbufferImage.nativeHandle);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 config.windowResolution.x,
                 config.windowResolution.y,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    backbufferImage.depth = 1;
    backbufferImage.format = Format::RGBA8;
    backbufferImage.type = ImageHandle::Type::TYPE_2D;
    backbufferImage.width = config.windowResolution.x;
    backbufferImage.height = config.windowResolution.y;

    backbufferView.image = &backbufferImage;
    backbufferView.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    backbufferView.subresourceRange.baseArrayLayer = 0;
    backbufferView.subresourceRange.baseMipLevel = 0;
    backbufferView.subresourceRange.layerCount = 1;
    backbufferView.subresourceRange.levelCount = 1;

    glBindFramebuffer(GL_FRAMEBUFFER, backbufferFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backbufferImage.nativeHandle, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RendererConfig Renderer::GetConfig()
{
    return this->config;
}

void Renderer::UpdateConfig(RendererConfig config)
{
    backbufferIsStale = true;
    this->config = config;
    SDL_SetWindowSize(window, config.windowResolution.x, config.windowResolution.y);
    // Hacky way to run the update on the rendering thread, maybe there should be an arbitrary functions RenderCommand
    renderQueueWrite.Push(RenderCommand(
        RenderCommand::CreateResourceParams([this](ResourceCreationContext & ctx) { UpdatePresentMode(); })));
}

RendererProperties const & Renderer::GetProperties()
{
    return properties;
}

void Renderer::CmdAbort(RenderCommand const & command)
{
    isAborting = true;
    abortCode = std::get<RenderCommand::AbortParams>(command.params).errorCode;
}

void Renderer::CmdCreateResources(RenderCommand const & command)
{
    OPTICK_EVENT();
    auto fun = std::get<RenderCommand::CreateResourceParams>(command.params).fun;
    OpenGLResourceContext ctx;
    glFlush();
    glFinish();
    fun(ctx);
    glFlush();
    glFinish();
}

void Renderer::CmdExecute(RenderCommand const & command)
{
    OPTICK_EVENT();
    auto params = std::get<RenderCommand::ExecuteCommandContextParams>(command.params);
    params.ctx->Execute(this, params.waitSem, params.signalSem, params.signalFence);
}

void Renderer::CmdSwapWindow(RenderCommand const & command)
{
    using namespace std::literals::chrono_literals;
    OPTICK_EVENT();
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
    glBlitNamedFramebuffer(backbufferFramebuffer,
                           0,
                           0,
                           0,
                           config.windowResolution.x,
                           config.windowResolution.y,
                           0,
                           0,
                           config.windowResolution.x,
                           config.windowResolution.y,
                           GL_COLOR_BUFFER_BIT,
                           GL_LINEAR);
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
}

#endif
