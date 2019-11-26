#include "RenderSystem.h"

#include "Core/Console/Console.h"
#include "Core/Logging/Logger.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"

static const auto logger = Logger::Create("RenderSystem");

RenderSystem::RenderSystem(Renderer * renderer) : renderer(renderer), uiRenderSystem(renderer)
{
    CommandDefinition backbufferOverrideCommand(
        "render_override_backbuffer",
        "render_override_backbuffer <imageview resource name> - Overrides the renderer's output to "
        "output the given imageview instead of the normal backbuffer. Call with the argument "
        "'false' to go back to normal rendering.",
        1,
        [this](auto args) {
            auto imageViewName = args[0];
            if (imageViewName == "false") {
                this->DebugOverrideBackbuffer(nullptr);
                return;
            }
            auto imageView = ResourceManager::GetResource<ImageViewHandle>(imageViewName);
            if (!imageView) {
                logger->Errorf("Could not find resource '%s'", imageView);
                return;
            }
            this->DebugOverrideBackbuffer(imageView);
        });
    Console::RegisterCommand(backbufferOverrideCommand);

    CommandDefinition refreshRenderpassCommand(
        "test_refresh_renderpass",
        "test_refresh_renderpass - calls SetRenderpass on all ShaderPrograms to ensure it works.",
        0,
        [this](auto args) {
            passthroughTransformProgram->SetRenderpass(mainRenderpass);
            postprocessProgram->SetRenderpass(postprocessRenderpass);
        });
    Console::RegisterCommand(refreshRenderpassCommand);
}

void RenderSystem::Init()
{
    mainRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass");
    postprocessRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass");

    passthroughTransformPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout");
    passthroughTransformProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/passthrough-transform.program");

    postprocessSampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/postprocess.sampler");
    postprocessDescriptorSetLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/postprocess.layout");
    postprocessLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout");
    postprocessProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/postprocess.program");

    quadEbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");
    quadVbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");

    InitSwapchainResources();

    uiRenderSystem.Init();
}

void RenderSystem::InitSwapchainResources()
{
    frameInfo.clear();
    frameInfo.resize(renderer->GetSwapCount());
    Semaphore sem;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        for (size_t i = 0; i < frameInfo.size(); ++i) {
            frameInfo[i].commandBufferAllocator = ctx.CreateCommandBufferAllocator();
            frameInfo[i].canStartFrame = ctx.CreateFence(true);
            frameInfo[i].framebufferReady = ctx.CreateSemaphore();
            frameInfo[i].preRenderPassFinished = ctx.CreateSemaphore();
            frameInfo[i].mainRenderPassFinished = ctx.CreateSemaphore();
            frameInfo[i].postprocessFinished = ctx.CreateSemaphore();
        }
        sem.Signal();
    });
    sem.Wait();

    auto framebuffers = renderer->CreateBackbuffers(mainRenderpass);
    CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
    ctxCreateInfo.level = CommandBufferLevel::PRIMARY;
    for (size_t i = 0; i < frameInfo.size(); ++i) {
        frameInfo[i].framebuffer = framebuffers[i];
        frameInfo[i].preRenderPassCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        frameInfo[i].mainCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        frameInfo[i].postProcessCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
    }
}
