#include "RenderSystem.h"

#include "Console/Console.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"
#include "Logging/Logger.h"
#include "RenderingBackend/Renderer.h"

static const auto logger = Logger::Create("RenderSystem");

float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

// Returns a float between 0.0 and 1.0
float RandomFloat()
{
    return ((float)rand() / (float)RAND_MAX);
}

RenderSystem::RenderSystem(Renderer * renderer)
    : jobEngine(JobEngine::GetInstance()), renderer(renderer), rendererProperties(renderer->GetProperties()),
      uiRenderSystem(renderer)
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
                logger.Error("Could not find resource '{}'", imageView);
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
                                            logger.Error("width {} is not a number", width);
                                        }

                                        auto heightInt = std::strtol(height.c_str(), nullptr, 0);
                                        if (heightInt == 0 && height != "0") {
                                            logger.Error("height {} is not a number", height);
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
                logger.Error("Unknown present mode '{}'", modeString);
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

    skeletalPrepassPipelineLayout = ResourceManager::GetResource<PipelineLayoutHandle>(
        "_Primitives/PipelineLayouts/prepass_skeletal.pipelinelayout");

    passthroughTransformPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout");
    passthroughTransformProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/passthrough-transform.program");

    lightsLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/lightsMesh.layout");

    meshModelLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/modelMesh.layout");
    meshPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh.pipelinelayout");
    meshProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/mesh.program");
    transparentMeshProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/TransparentMesh.program");

    skeletalMeshBoneLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/boneMesh.layout");
    skeletalMeshPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh_skeletal.pipelinelayout");
    skeletalMeshProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/mesh_skeletal.program");

    ambientOcclusionDescriptorSetLayout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
        "_Primitives/DescriptorSetLayouts/ambientOcclusion.layout");
    ambientOcclusionLayout = ResourceManager::GetResource<PipelineLayoutHandle>(
        "_Primitives/PipelineLayouts/ambientOcclusion.pipelinelayout");
    ambientOcclusionProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/ambientOcclusion.program");

    ambientOcclusionBlurDescriptorSetLayout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
        "_Primitives/DescriptorSetLayouts/ambientOcclusionBlur.layout");
    ambientOcclusionBlurLayout = ResourceManager::GetResource<PipelineLayoutHandle>(
        "_Primitives/PipelineLayouts/ambientOcclusionBlur.pipelinelayout");
    ambientOcclusionBlurProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/ambientOcclusionBlur.program");

    tonemapDescriptorSetLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/tonemap.layout");
    tonemapLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/tonemap.pipelinelayout");
    tonemapProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/tonemap.program");

    postprocessSampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/postprocess.sampler");
    postprocessDescriptorSetLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/postprocess.layout");
    postprocessLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout");
    postprocessProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/postprocess.program");

    uiProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/ui.program");

    debugDrawLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/debug_draw.pipelinelayout");
    debugDrawLinesProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/debug_draw_lines.program");
    debugDrawPointsProgram =
        ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/debug_draw_points.program");

    quadEbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");
    quadVbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");

    ssaoPass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/ssao.pass");

    Semaphore sem;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        {
            std::vector<uint8_t> noiseTexture(16 * sizeof(glm::vec4));
            for (size_t j = 0; j < 16; ++j) {
                glm::vec4 noise(RandomFloat() * 2.f - 1.f, RandomFloat() * 2.f - 1.f, 0.f, 1.f);
                memcpy(&noiseTexture[j * sizeof(glm::vec4)], &noise, sizeof(glm::vec4));
            }

            ResourceCreationContext::ImageCreateInfo aoNoiseCi;
            aoNoiseCi.depth = 1;
            aoNoiseCi.width = 4;
            aoNoiseCi.height = 4;
            aoNoiseCi.format = Format::R32G32B32A32_SFLOAT;
            aoNoiseCi.mipLevels = 1;
            aoNoiseCi.type = ImageHandle::Type::TYPE_2D;
            aoNoiseCi.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT |
                              ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
            ssaoNoiseImage = ctx.CreateImage(aoNoiseCi);
            ctx.ImageData(ssaoNoiseImage, noiseTexture);

            ResourceCreationContext::ImageViewCreateInfo aoNoiseImgViewCi;
            aoNoiseImgViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
            aoNoiseImgViewCi.format = Format::R32G32B32A32_SFLOAT;
            aoNoiseImgViewCi.image = ssaoNoiseImage;
            aoNoiseImgViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
            aoNoiseImgViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
            aoNoiseImgViewCi.subresourceRange.baseArrayLayer = 0;
            aoNoiseImgViewCi.subresourceRange.baseMipLevel = 0;
            aoNoiseImgViewCi.subresourceRange.layerCount = 1;
            aoNoiseImgViewCi.subresourceRange.levelCount = 1;
            ssaoNoiseImageView = ctx.CreateImageView(aoNoiseImgViewCi);
        }

        sem.Signal();
    });
    sem.Wait();

    InitSwapchainResources();

    uiRenderSystem.Init();
}

void RenderSystem::InitSwapchainResources()
{
    logger.Info("InitSwapchainResources");
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
                Format::R32G32B32A32_SFLOAT,
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
                RenderPassHandle::AttachmentDescription::StoreOp::STORE,
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

        {
            std::vector<RenderPassHandle::AttachmentDescription> ssaoAttachments = {
                {0,
                 Format::R32_SFLOAT,
                 RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                 RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                 RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                 RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                 ImageLayout::UNDEFINED,
                 ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            };
            RenderPassHandle::AttachmentReference outputReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
            RenderPassHandle::SubpassDescription subpass = {
                RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {outputReference}, {}, {}, {}};
            ResourceCreationContext::RenderPassCreateInfo ssaoRpCi;
            ssaoRpCi.attachments = ssaoAttachments;
            ssaoRpCi.subpasses = {subpass};
            if (this->ssaoPass) {
                ctx.DestroyRenderPass(this->ssaoPass);
            }
            this->ssaoPass = ctx.CreateRenderPass(ssaoRpCi);
            ResourceManager::AddResource("_Primitives/Renderpasses/ssao.pass", ssaoPass);
        }

        std::vector<RenderPassHandle::AttachmentDescription> postprocessAttachments = {
            // Backbuffer
            {0,
             renderer->GetBackbufferFormat(),
             // TODO: Can probably be DONT_CARE once we're doing actual post processing
             RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
             RenderPassHandle::AttachmentDescription::StoreOp::STORE,
             RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
             RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
             ImageLayout::UNDEFINED,
             ImageLayout::PRESENT_SRC_KHR},
            // Blurred SSAO
            {0,
             Format::R32_SFLOAT,
             RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
             RenderPassHandle::AttachmentDescription::StoreOp::STORE,
             RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
             RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
             ImageLayout::UNDEFINED,
             ImageLayout::COLOR_ATTACHMENT_OPTIMAL}};

        RenderPassHandle::AttachmentReference backbufferReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        RenderPassHandle::AttachmentReference aoBlurColorRef = {1, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        RenderPassHandle::AttachmentReference aoBlurInputRef = {1, ImageLayout::SHADER_READ_ONLY_OPTIMAL};

        RenderPassHandle::SubpassDescription aoBlurSubpass = {
            RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {aoBlurColorRef}, {}, {}, {}};

        RenderPassHandle::SubpassDescription postprocessSubpass = {
            RenderPassHandle::PipelineBindPoint::GRAPHICS, {aoBlurInputRef}, {backbufferReference}, {}, {}, {}};

        RenderPassHandle::SubpassDependency aoBlurDependency = {
            0, 1, PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT, PipelineStageFlagBits::TOP_OF_PIPE_BIT, 0, 0, 0};

        ResourceCreationContext::RenderPassCreateInfo postprocessPassInfo = {
            postprocessAttachments, {aoBlurSubpass, postprocessSubpass}, {aoBlurDependency}};

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
            if (fi.hdrColorBufferImageView) {
                ctx.DestroyImageView(fi.hdrColorBufferImageView);
            }
            if (fi.hdrColorBufferImage) {
                ctx.DestroyImage(fi.hdrColorBufferImage);
            }
            if (fi.normalsGBufferImageView) {
                ctx.DestroyImageView(fi.normalsGBufferImageView);
            }
            if (fi.normalsGBufferImage) {
                ctx.DestroyImage(fi.normalsGBufferImage);
            }
            if (fi.ssaoDescriptorSet) {
                ctx.DestroyDescriptorSet(fi.ssaoDescriptorSet);
            }
            if (fi.tonemapDescriptorSet) {
                ctx.DestroyDescriptorSet(fi.tonemapDescriptorSet);
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
            {
                PrimitiveTopology::TRIANGLE_LIST,
            },
            depthStencil);
    } else {
        prepassProgram->SetRenderpass(prepass);
    }

    if (!skeletalPrepassProgram) {
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
        depthStencil.depthCompareOp = CompareOp::LESS;
        depthStencil.depthTestEnable = true;
        depthStencil.depthWriteEnable = true;
        skeletalPrepassProgram = ShaderProgram::Create(
            "_Primitives/ShaderPrograms/prepass_skeletal.program",
            {"shaders/prepass_skeletal.vert"},
            ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/mesh_skeletal.state"),
            ResourceManager::GetResource<PipelineLayoutHandle>(
                "_Primitives/PipelineLayouts/prepass_skeletal.pipelinelayout"),
            prepass,
            CullMode::BACK,
            FrontFace::COUNTER_CLOCKWISE,
            0,
            {},
            {
                PrimitiveTopology::TRIANGLE_LIST,
            },
            depthStencil);
    } else {
        skeletalPrepassProgram->SetRenderpass(prepass);
    }

    passthroughTransformProgram->SetRenderpass(mainRenderpass);
    postprocessProgram->SetRenderpass(postprocessRenderpass);
    uiProgram->SetRenderpass(postprocessRenderpass);
}

void RenderSystem::InitFramebuffers(ResourceCreationContext & ctx)
{
    auto res = renderer->GetResolution();

    std::vector<glm::vec4> ssaoSamples(MAX_SSAO_SAMPLES);
    {
        for (size_t j = 0; j < MAX_SSAO_SAMPLES; ++j) {
            auto & sample = ssaoSamples[j];
            auto v3 = glm::vec3(RandomFloat() * 2.f - 1.f, RandomFloat() * 2.f - 1.f, RandomFloat());
            v3 = glm::normalize(v3);
            v3 *= RandomFloat();
            float scale = (float)j / 64.f;
            scale = Lerp(0.1f, 1.f, scale * scale);
            v3 *= scale;

            sample = glm::vec4(v3, 1.f);
        }
    }

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

        ResourceCreationContext::ImageCreateInfo hdrColorBufferImageCi;
        hdrColorBufferImageCi.depth = 1;
        hdrColorBufferImageCi.width = res.x;
        hdrColorBufferImageCi.height = res.y;
        hdrColorBufferImageCi.format = Format::R32G32B32A32_SFLOAT;
        hdrColorBufferImageCi.mipLevels = 1;
        hdrColorBufferImageCi.type = ImageHandle::Type::TYPE_2D;
        hdrColorBufferImageCi.usage =
            IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT;
        frameInfo[i].hdrColorBufferImage = ctx.CreateImage(hdrColorBufferImageCi);
        ctx.AllocateImage(frameInfo[i].hdrColorBufferImage);

        ResourceCreationContext::ImageViewCreateInfo hdrColorBufferImageViewCi;
        hdrColorBufferImageViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
        hdrColorBufferImageViewCi.format = Format::R32G32B32A32_SFLOAT;
        hdrColorBufferImageViewCi.image = frameInfo[i].hdrColorBufferImage;
        hdrColorBufferImageViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
        hdrColorBufferImageViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
        hdrColorBufferImageViewCi.subresourceRange.baseArrayLayer = 0;
        hdrColorBufferImageViewCi.subresourceRange.baseMipLevel = 0;
        hdrColorBufferImageViewCi.subresourceRange.layerCount = 1;
        hdrColorBufferImageViewCi.subresourceRange.levelCount = 1;
        frameInfo[i].hdrColorBufferImageView = ctx.CreateImageView(hdrColorBufferImageViewCi);
        ResourceManager::AddResource(std::string("/_RenderSystem/frameInfo/") + std::to_string(i) +
                                         "/hdrColorBuffer.imageview",
                                     frameInfo[i].hdrColorBufferImageView);

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
        mainFramebufferCi.attachments = {frameInfo[i].hdrColorBufferImageView,
                                         frameInfo[i].normalsGBufferImageView,
                                         frameInfo[i].prepassDepthImageView};
        mainFramebufferCi.width = res.x;
        mainFramebufferCi.height = res.y;
        mainFramebufferCi.layers = 1;
        mainFramebufferCi.renderPass = this->mainRenderpass;
        frameInfo[i].framebuffer = ctx.CreateFramebuffer(mainFramebufferCi);

        {
            ResourceCreationContext::ImageCreateInfo aoImgCi;
            aoImgCi.depth = 1;
            aoImgCi.width = res.x;
            aoImgCi.height = res.y;
            aoImgCi.format = Format::R32_SFLOAT;
            aoImgCi.mipLevels = 1;
            aoImgCi.type = ImageHandle::Type::TYPE_2D;
            aoImgCi.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT |
                            ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT;
            frameInfo[i].ssaoOutputImage = ctx.CreateImage(aoImgCi);
            ctx.AllocateImage(frameInfo[i].ssaoOutputImage);

            ResourceCreationContext::ImageViewCreateInfo aoImgViewCi;
            aoImgViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
            aoImgViewCi.format = Format::R32_SFLOAT;
            aoImgViewCi.image = frameInfo[i].ssaoOutputImage;
            aoImgViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
            aoImgViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
            aoImgViewCi.subresourceRange.baseArrayLayer = 0;
            aoImgViewCi.subresourceRange.baseMipLevel = 0;
            aoImgViewCi.subresourceRange.layerCount = 1;
            aoImgViewCi.subresourceRange.levelCount = 1;
            frameInfo[i].ssaoOutputImageView = ctx.CreateImageView(aoImgViewCi);
        }

        {
            ResourceCreationContext::ImageCreateInfo aoBlurImgCi;
            aoBlurImgCi.depth = 1;
            aoBlurImgCi.width = res.x;
            aoBlurImgCi.height = res.y;
            aoBlurImgCi.format = Format::R32_SFLOAT;
            aoBlurImgCi.mipLevels = 1;
            aoBlurImgCi.type = ImageHandle::Type::TYPE_2D;
            aoBlurImgCi.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_COLOR_ATTACHMENT_BIT |
                                ImageUsageFlagBits::IMAGE_USAGE_FLAG_INPUT_ATTACHMENT_BIT;
            frameInfo[i].ssaoBlurImage = ctx.CreateImage(aoBlurImgCi);
            ctx.AllocateImage(frameInfo[i].ssaoBlurImage);

            ResourceCreationContext::ImageViewCreateInfo aoBlurImgViewCi;
            aoBlurImgViewCi.components = ImageViewHandle::ComponentMapping::IDENTITY;
            aoBlurImgViewCi.format = Format::R32_SFLOAT;
            aoBlurImgViewCi.image = frameInfo[i].ssaoBlurImage;
            aoBlurImgViewCi.viewType = ImageViewHandle::Type::TYPE_2D;
            aoBlurImgViewCi.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
            aoBlurImgViewCi.subresourceRange.baseArrayLayer = 0;
            aoBlurImgViewCi.subresourceRange.baseMipLevel = 0;
            aoBlurImgViewCi.subresourceRange.layerCount = 1;
            aoBlurImgViewCi.subresourceRange.levelCount = 1;
            frameInfo[i].ssaoBlurImageView = ctx.CreateImageView(aoBlurImgViewCi);
        }

        {
            ResourceCreationContext::FramebufferCreateInfo ssaoFbCi;
            ssaoFbCi.attachments = {frameInfo[i].ssaoOutputImageView};
            ssaoFbCi.width = res.x;
            ssaoFbCi.height = res.y;
            ssaoFbCi.layers = 1;
            ssaoFbCi.renderPass = this->ssaoPass;
            frameInfo[i].ssaoFramebuffer = ctx.CreateFramebuffer(ssaoFbCi);
        }

        ResourceCreationContext::FramebufferCreateInfo postprocessFramebufferCi;
        postprocessFramebufferCi.attachments = {frameInfo[i].backbuffer, frameInfo[i].ssaoBlurImageView};
        postprocessFramebufferCi.width = res.x;
        postprocessFramebufferCi.height = res.y;
        postprocessFramebufferCi.layers = 1;
        postprocessFramebufferCi.renderPass = this->postprocessRenderpass;
        frameInfo[i].postprocessFramebuffer = ctx.CreateFramebuffer(postprocessFramebufferCi);

        {
            ResourceCreationContext::BufferCreateInfo ssaoParamBufferCi;
            ssaoParamBufferCi.memoryProperties =
                MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT;
            ssaoParamBufferCi.size = sizeof(GpuSsaoParameters);
            ssaoParamBufferCi.usage = BufferUsageFlags::UNIFORM_BUFFER_BIT;
            frameInfo[i].ssaoParameterBuffer = ctx.CreateBuffer(ssaoParamBufferCi);
            frameInfo[i].ssaoParameterBufferMapped =
                (GpuSsaoParameters *)ctx.MapBuffer(frameInfo[i].ssaoParameterBuffer, 0, sizeof(GpuSsaoParameters));
            for (size_t j = 0; j < MAX_SSAO_SAMPLES; ++j) {
                frameInfo[i].ssaoParameterBufferMapped->samples[j] = ssaoSamples[j];
            }
        }

        {
            ResourceCreationContext::DescriptorSetCreateInfo ambientOcclusionDSCi;
            // TODO: Should use half resolution image views (mip?)
            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor ambientOcclusionImgDescs[] = {
                {postprocessSampler, frameInfo[i].normalsGBufferImageView},
                {postprocessSampler, frameInfo[i].prepassDepthImageView},
                {postprocessSampler, ssaoNoiseImageView}};
            ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor ambientOcclusionBufferDescs[] = {
                {frameInfo[i].ssaoParameterBuffer, 0, sizeof(GpuSsaoParameters)}};
            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor ambientOcclusionDescs[] = {
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, ambientOcclusionImgDescs[0]},
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ambientOcclusionImgDescs[1]},
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 2, ambientOcclusionImgDescs[2]},
                {DescriptorType::UNIFORM_BUFFER, 3, ambientOcclusionBufferDescs[0]}};
            ambientOcclusionDSCi.descriptorCount = 4;
            ambientOcclusionDSCi.layout = ambientOcclusionDescriptorSetLayout;
            ambientOcclusionDSCi.descriptors = ambientOcclusionDescs;
            frameInfo[i].ssaoDescriptorSet = ctx.CreateDescriptorSet(ambientOcclusionDSCi);
        }

        {
            ResourceCreationContext::DescriptorSetCreateInfo ambientOcclusionBlurDSCi;
            // TODO: Should use half resolution image views (mip?)
            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor ambientOcclusionBlurImgDescs[] = {
                {postprocessSampler, frameInfo[i].ssaoOutputImageView}};
            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor ambientOcclusionBlurDescs[] = {
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, ambientOcclusionBlurImgDescs[0]}};
            ambientOcclusionBlurDSCi.descriptorCount = 1;
            ambientOcclusionBlurDSCi.layout = ambientOcclusionBlurDescriptorSetLayout;
            ambientOcclusionBlurDSCi.descriptors = ambientOcclusionBlurDescs;
            frameInfo[i].ssaoBlurDescriptorSet = ctx.CreateDescriptorSet(ambientOcclusionBlurDSCi);
        }

        {
            ResourceCreationContext::DescriptorSetCreateInfo tonemapDSCi;
            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor tonemapImgDescs[] = {
                {postprocessSampler, frameInfo[i].hdrColorBufferImageView}, {nullptr, frameInfo[i].ssaoBlurImageView}};
            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor tonemapDescs[] = {
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, tonemapImgDescs[0]},
                {DescriptorType::INPUT_ATTACHMENT, 1, tonemapImgDescs[1]}};
            tonemapDSCi.descriptorCount = 2;
            tonemapDSCi.layout = tonemapDescriptorSetLayout;
            tonemapDSCi.descriptors = tonemapDescs;
            frameInfo[i].tonemapDescriptorSet = ctx.CreateDescriptorSet(tonemapDSCi);
        }
    }
}
