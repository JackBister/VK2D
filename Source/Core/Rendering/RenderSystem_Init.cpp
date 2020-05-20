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

    RenderSystem::instance = this;
}

void RenderSystem::Init()
{
    mainRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass");
    postprocessRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass");

    prepassPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/prepass.pipelinelayout");

    passthroughTransformPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout");
    passthroughTransformProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/passthrough-transform.program");

    meshModelLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/modelMesh.layout");
    meshPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh.pipelinelayout");
    meshProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/mesh.program");
    transparentMeshProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/TransparentMesh.program");

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
        std::vector<RenderPassHandle::AttachmentDescription> prepassDepthAttachments = {
            {0,
             Format::D32_SFLOAT,
             RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
             RenderPassHandle::AttachmentDescription::StoreOp::STORE,
             RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
             RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
             ImageLayout::UNDEFINED,
             ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL}};
        RenderPassHandle::AttachmentReference prepassReference = {0, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        RenderPassHandle::SubpassDescription prepassSubpass = {
            RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {}, {}, prepassReference, {}};
        ResourceCreationContext::RenderPassCreateInfo prepassInfo = {prepassDepthAttachments, {prepassSubpass}, {}};
        if (this->prepass) {
            ctx.DestroyRenderPass(this->prepass);
        }
        this->prepass = ctx.CreateRenderPass(prepassInfo);

        // TODO: This is ugly. The RenderPrimitiveFactory creates initial versions of the render passes which are then
        // immediately replaced here. RenderPrimitiveFactory was always ugly but having all this resource creation code
        // inline in RenderSystem is also ugly. Need to find a compromise.
        std::vector<RenderPassHandle::AttachmentDescription> attachments = {
            {
                0,
                renderer->GetBackbufferFormat(),
                RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
                RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                ImageLayout::UNDEFINED,
                ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            },
            {
                0,
                Format::R16G16_SFLOAT,
                RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
                RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                ImageLayout::UNDEFINED,
                ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            },
            {
                0,
                Format::D32_SFLOAT,
                RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            }};

        RenderPassHandle::AttachmentReference colorReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        RenderPassHandle::AttachmentReference normalsGBufferReference = {1, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        RenderPassHandle::AttachmentReference depthReference = {2, ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL};

        RenderPassHandle::SubpassDescription mainpassSubpass = {RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                                {},
                                                                {colorReference, normalsGBufferReference},
                                                                {},
                                                                depthReference,
                                                                {}};

        ResourceCreationContext::RenderPassCreateInfo passInfo = {attachments, {mainpassSubpass}, {}};
        if (this->mainRenderpass) {
            ctx.DestroyRenderPass(this->mainRenderpass);
        }
        this->mainRenderpass = ctx.CreateRenderPass(passInfo);
        ResourceManager::AddResource("_Primitives/Renderpasses/main.pass", this->mainRenderpass);

        std::vector<RenderPassHandle::AttachmentDescription> postprocessAttachments = {
            {0,
             renderer->GetBackbufferFormat(),
             // TODO: Can probably be DONT_CARE once we're doing actual post processing
             RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
             RenderPassHandle::AttachmentDescription::StoreOp::STORE,
             RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
             RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
             ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
             ImageLayout::PRESENT_SRC_KHR}};

        RenderPassHandle::AttachmentReference postprocessReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        RenderPassHandle::SubpassDescription postprocessSubpass = {
            RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {postprocessReference}, {}, {}, {}};

        ResourceCreationContext::RenderPassCreateInfo postprocessPassInfo = {
            postprocessAttachments, {postprocessSubpass}, {}};
        if (this->postprocessRenderpass) {
            ctx.DestroyRenderPass(this->postprocessRenderpass);
        }
        this->postprocessRenderpass = ctx.CreateRenderPass(postprocessPassInfo);
        ResourceManager::AddResource("_Primitives/Renderpasses/postprocess.pass", this->postprocessRenderpass);

        for (auto & fi : frameInfo) {
            if (fi.prepassFramebuffer) {
                ctx.DestroyFramebuffer(fi.prepassFramebuffer);
            }
            if (fi.prepassDepthImageView) {
                ctx.DestroyImageView(fi.prepassDepthImageView);
            }
            if (fi.prepassDepthImage) {
                ctx.DestroyImage(fi.prepassDepthImage);
            }
            if (fi.framebuffer) {
                ctx.DestroyFramebuffer(fi.framebuffer);
            }
            if (fi.normalsGBufferImageView) {
                ctx.DestroyImageView(fi.normalsGBufferImageView);
            }
            if (fi.normalsGBufferImage) {
                ctx.DestroyImage(fi.normalsGBufferImage);
            }
            if (fi.postprocessFramebuffer) {
                ctx.DestroyFramebuffer(fi.postprocessFramebuffer);
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

    if (!prepassProgram) {
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
        depthStencil.depthCompareOp = CompareOp::LESS;
        depthStencil.depthTestEnable = true;
        depthStencil.depthWriteEnable = true;
        prepassProgram = ShaderProgram::Create(
            "_Primitives/ShaderPrograms/prepass.program",
            {"shaders/prepass.vert"},
            ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/mesh.state"),
            ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/prepass.pipelinelayout"),
            prepass,
            CullMode::BACK,
            FrontFace::COUNTER_CLOCKWISE,
            0,
            {},
            depthStencil);
    } else {
        prepassProgram->SetRenderpass(prepass);
    }

    passthroughTransformProgram->SetRenderpass(mainRenderpass);
    postprocessProgram->SetRenderpass(postprocessRenderpass);
    uiProgram->SetRenderpass(postprocessRenderpass);
}

void RenderSystem::InitFramebuffers(ResourceCreationContext & ctx)
{
    auto res = renderer->GetResolution();
    for (size_t i = 0; i < frameInfo.size(); ++i) {
        ResourceCreationContext::ImageCreateInfo prepassDepthImageCi;
        prepassDepthImageCi.depth = 1;
        prepassDepthImageCi.width = res.x;
        prepassDepthImageCi.height = res.y;
        prepassDepthImageCi.format = Format::D32_SFLOAT;
        prepassDepthImageCi.mipLevels = 1;
        prepassDepthImageCi.type = ImageHandle::Type::TYPE_2D;
        prepassDepthImageCi.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_DEPTH_STENCIL_ATTACHMENT_BIT |
                                    ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT;
        frameInfo[i].prepassDepthImage = ctx.CreateImage(prepassDepthImageCi);
        ctx.AllocateImage(frameInfo[i].prepassDepthImage);

        ResourceCreationContext::ImageViewCreateInfo prepassDepthImageViewCi;
        prepassDepthImageViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
        prepassDepthImageViewCi.format = Format::D32_SFLOAT;
        prepassDepthImageViewCi.image = frameInfo[i].prepassDepthImage;
        prepassDepthImageViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
        prepassDepthImageViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::DEPTH_BIT;
        prepassDepthImageViewCi.subresourceRange.baseArrayLayer = 0;
        prepassDepthImageViewCi.subresourceRange.baseMipLevel = 0;
        prepassDepthImageViewCi.subresourceRange.layerCount = 1;
        prepassDepthImageViewCi.subresourceRange.levelCount = 1;
        frameInfo[i].prepassDepthImageView = ctx.CreateImageView(prepassDepthImageViewCi);
        ResourceManager::AddResource(std::string("/_RenderSystem/frameInfo/") + std::to_string(i) +
                                         "/prepass/depth.imageview",
                                     frameInfo[i].prepassDepthImageView);

        ResourceCreationContext::FramebufferCreateInfo prepassFramebufferCi;
        prepassFramebufferCi.attachments = {frameInfo[i].prepassDepthImageView};
        prepassFramebufferCi.width = res.x;
        prepassFramebufferCi.height = res.y;
        prepassFramebufferCi.layers = 1;
        prepassFramebufferCi.renderPass = this->prepass;
        frameInfo[i].prepassFramebuffer = ctx.CreateFramebuffer(prepassFramebufferCi);

        ResourceCreationContext::ImageCreateInfo normalsGBufferImageCi;
        normalsGBufferImageCi.depth = 1;
        normalsGBufferImageCi.width = res.x;
        normalsGBufferImageCi.height = res.y;
        normalsGBufferImageCi.format = Format::R16G16_SFLOAT;
        normalsGBufferImageCi.mipLevels = 1;
        normalsGBufferImageCi.type = ImageHandle::Type::TYPE_2D;
        normalsGBufferImageCi.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT |
                                      ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT;
        frameInfo[i].normalsGBufferImage = ctx.CreateImage(normalsGBufferImageCi);
        ctx.AllocateImage(frameInfo[i].normalsGBufferImage);

        ResourceCreationContext::ImageViewCreateInfo normalsGBufferImageViewCi;
        normalsGBufferImageViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
        normalsGBufferImageViewCi.format = Format::R16G16_SFLOAT;
        normalsGBufferImageViewCi.image = frameInfo[i].normalsGBufferImage;
        normalsGBufferImageViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
        normalsGBufferImageViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
        normalsGBufferImageViewCi.subresourceRange.baseArrayLayer = 0;
        normalsGBufferImageViewCi.subresourceRange.baseMipLevel = 0;
        normalsGBufferImageViewCi.subresourceRange.layerCount = 1;
        normalsGBufferImageViewCi.subresourceRange.levelCount = 1;
        frameInfo[i].normalsGBufferImageView = ctx.CreateImageView(normalsGBufferImageViewCi);
        ResourceManager::AddResource(std::string("/_RenderSystem/frameInfo/") + std::to_string(i) +
                                         "/normalsGBuffer.imageview",
                                     frameInfo[i].normalsGBufferImageView);

        ResourceCreationContext::FramebufferCreateInfo mainFramebufferCi;
        mainFramebufferCi.attachments = {
            frameInfo[i].backbuffer, frameInfo[i].normalsGBufferImageView, frameInfo[i].prepassDepthImageView};
        mainFramebufferCi.width = res.x;
        mainFramebufferCi.height = res.y;
        mainFramebufferCi.layers = 1;
        mainFramebufferCi.renderPass = this->mainRenderpass;
        frameInfo[i].framebuffer = ctx.CreateFramebuffer(mainFramebufferCi);

        ResourceCreationContext::FramebufferCreateInfo postprocessFramebufferCi;
        postprocessFramebufferCi.attachments = {frameInfo[i].backbuffer};
        postprocessFramebufferCi.width = res.x;
        postprocessFramebufferCi.height = res.y;
        postprocessFramebufferCi.layers = 1;
        postprocessFramebufferCi.renderPass = this->postprocessRenderpass;
        frameInfo[i].postprocessFramebuffer = ctx.CreateFramebuffer(postprocessFramebufferCi);
    }
}
