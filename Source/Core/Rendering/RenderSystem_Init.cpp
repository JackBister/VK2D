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

    CommandDefinition resizeCommand("set_window_resolution",
                                    "set_window_resolution <width> <height> - set the resolution of the window",
                                    2,
                                    [this](auto args) {
                                        auto width = args[0];
                                        auto height = args[1];

                                        auto widthInt = std::strtol(width.c_str(), nullptr, 0);
                                        if (widthInt == 0 && width != "0") {
                                            logger->Errorf("width %s is not a number", width.c_str());
                                        }

                                        auto heightInt = std::strtol(height.c_str(), nullptr, 0);
                                        if (heightInt == 0 && height != "0") {
                                            logger->Errorf("height %s is not a number", height.c_str());
                                        }
                                        auto config = this->renderer->GetConfig();
                                        config.windowResolution.x = widthInt;
                                        config.windowResolution.y = heightInt;
                                        this->queuedConfigUpdate = config;
                                    });
    Console::RegisterCommand(resizeCommand);

    CommandDefinition presentModeCommand(
        "set_present_mode",
        "set_present_mode <immediate|fifo|mailbox> - set the renderer's presentation mode",
        1,
        [this](auto args) {
            auto config = this->renderer->GetConfig();
            auto modeString = args[0];
            if (modeString == "immediate") {
                config.presentMode = PresentMode::IMMEDIATE;
            } else if (modeString == "fifo") {
                config.presentMode = PresentMode::FIFO;
            } else if (modeString == "mailbox") {
                config.presentMode = PresentMode::MAILBOX;
            } else {
                logger->Errorf("Unknown present mode '%s'", modeString.c_str());
                return;
            }
            this->queuedConfigUpdate = config;
        });
    Console::RegisterCommand(presentModeCommand);
}

void RenderSystem::Init()
{
    mainRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass");
    postprocessRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass");

    passthroughTransformPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout");
    passthroughTransformProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/passthrough-transform.program");

    meshPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh.pipelinelayout");
    meshProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/mesh.program");

    postprocessSampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/postprocess.sampler");
    postprocessDescriptorSetLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/postprocess.layout");
    postprocessLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout");
    postprocessProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/postprocess.program");

    uiProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/ui.program");

    quadEbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");
    quadVbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");

    InitSwapchainResources();

    uiRenderSystem.Init();
}

void RenderSystem::InitSwapchainResources()
{
    logger->Infof("InitSwapchainResources");
    Semaphore sem;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        // TODO: This is ugly. The RenderPrimitiveFactory creates initial versions of the render passes which are then
        // immediately replaced here. RenderPrimitiveFactory was always ugly but having all this resource creation code
        // inline in RenderSystem is also ugly. Need to find a compromise.
        RenderPassHandle::AttachmentDescription attachment = {
            0,
            renderer->GetBackbufferFormat(),
            RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
            RenderPassHandle::AttachmentDescription::StoreOp::STORE,
            RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
            RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
            ImageLayout::UNDEFINED,
            ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        RenderPassHandle::AttachmentReference reference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        RenderPassHandle::SubpassDescription subpass = {
            RenderPassHandle::PipelineBindPoint::GRAPHICS, 0, nullptr, 1, &reference, nullptr, nullptr, 0, nullptr};

        ResourceCreationContext::RenderPassCreateInfo passInfo = {1, &attachment, 1, &subpass, 0, nullptr};
        if (this->mainRenderpass) {
            ctx.DestroyRenderPass(this->mainRenderpass);
        }
        this->mainRenderpass = ctx.CreateRenderPass(passInfo);
        ResourceManager::AddResource("_Primitives/Renderpasses/main.pass", this->mainRenderpass);

        RenderPassHandle::AttachmentDescription postprocessAttachment = {
            0,
            renderer->GetBackbufferFormat(),
            // TODO: Can probably be DONT_CARE once we're doing actual post processing
            RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
            RenderPassHandle::AttachmentDescription::StoreOp::STORE,
            RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
            RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
            ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            ImageLayout::PRESENT_SRC_KHR};

        RenderPassHandle::AttachmentReference postprocessReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        RenderPassHandle::SubpassDescription postprocessSubpass = {RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                                   0,
                                                                   nullptr,
                                                                   1,
                                                                   &postprocessReference,
                                                                   nullptr,
                                                                   nullptr,
                                                                   0,
                                                                   nullptr};

        ResourceCreationContext::RenderPassCreateInfo postprocessPassInfo = {
            1, &postprocessAttachment, 1, &postprocessSubpass, 0, nullptr};
        if (this->postprocessRenderpass) {
            ctx.DestroyRenderPass(this->postprocessRenderpass);
        }
        this->postprocessRenderpass = ctx.CreateRenderPass(postprocessPassInfo);
        ResourceManager::AddResource("_Primitives/Renderpasses/postprocess.pass", this->postprocessRenderpass);

        for (auto & fi : frameInfo) {
            if (fi.framebuffer) {
                ctx.DestroyFramebuffer(fi.framebuffer);
            }
            if (fi.commandBufferAllocator) {
                ctx.DestroyCommandBufferAllocator(fi.commandBufferAllocator);
            }
            if (fi.canStartFrame) {
                ctx.DestroyFence(fi.canStartFrame);
            }
            if (fi.framebufferReady) {
                ctx.DestroySemaphore(fi.framebufferReady);
            }
            if (fi.preRenderPassFinished) {
                ctx.DestroySemaphore(fi.preRenderPassFinished);
            }
            if (fi.mainRenderPassFinished) {
                ctx.DestroySemaphore(fi.mainRenderPassFinished);
            }
            if (fi.postprocessFinished) {
                ctx.DestroySemaphore(fi.postprocessFinished);
            }
        }

        auto backbuffers = renderer->GetBackbuffers();
        CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
        ctxCreateInfo.level = CommandBufferLevel::PRIMARY;

        frameInfo.clear();
        frameInfo.resize(renderer->GetSwapCount());
        for (size_t i = 0; i < frameInfo.size(); ++i) {
            frameInfo[i].commandBufferAllocator = ctx.CreateCommandBufferAllocator();
            frameInfo[i].canStartFrame = ctx.CreateFence(true);
            frameInfo[i].framebufferReady = ctx.CreateSemaphore();
            frameInfo[i].preRenderPassFinished = ctx.CreateSemaphore();
            frameInfo[i].mainRenderPassFinished = ctx.CreateSemaphore();
            frameInfo[i].postprocessFinished = ctx.CreateSemaphore();

            frameInfo[i].backbuffer = backbuffers[i];
            frameInfo[i].preRenderPassCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
            frameInfo[i].mainCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
            frameInfo[i].postProcessCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        }

        InitFramebuffers(ctx);
        sem.Signal();
    });
    sem.Wait();

    passthroughTransformProgram->SetRenderpass(mainRenderpass);
    postprocessProgram->SetRenderpass(postprocessRenderpass);
    uiProgram->SetRenderpass(mainRenderpass);
}

void RenderSystem::InitFramebuffers(ResourceCreationContext & ctx)
{
    auto res = renderer->GetResolution();
    for (size_t i = 0; i < frameInfo.size(); ++i) {
        ResourceCreationContext::FramebufferCreateInfo ci;
        ci.attachments = {frameInfo[i].backbuffer};
        ci.width = res.x;
        ci.height = res.y;
        ci.layers = 1;
        ci.renderPass = this->mainRenderpass;

        frameInfo[i].framebuffer = ctx.CreateFramebuffer(ci);
    }
}
