#include "Core/Rendering/RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>

#include "Core/Rendering/Shaders/passthrough-transform.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough.vert.spv.h"
#include "Core/Rendering/Shaders/ui.frag.spv.h"
#include "Core/Rendering/Shaders/ui.vert.spv.h"
#include "Core/entity.h"

static constexpr CommandBuffer::ClearValue DEFAULT_CLEAR_VALUES[] = {
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}}};

RenderSystem::RenderSystem(Renderer * renderer, ResourceManager * resourceManager)
    : renderer(renderer), resourceManager(resourceManager), frameInfo(renderer->GetSwapCount()),
      uiRenderSystem(renderer)
{
    {
        using namespace std::chrono_literals;
        std::vector<float> const plainQuadVerts{
            // vec3 pos, vec3 color, vec2 texcoord
            -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
            1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
            -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
        };

        std::vector<uint32_t> const plainQuadElems = {0, 1, 2, 2, 3, 0};

        std::atomic_int finishedJobs = 0;

        DescriptorSetLayoutHandle * cameraPtLayout;
        DescriptorSetLayoutHandle * spritePtLayout;

        DescriptorSetLayoutHandle * uiPipelineDescriptorSetLayout;

        PipelineLayoutHandle * uiLayout;

        ShaderModuleHandle * passthroughNoTransformVertShader;
        ShaderModuleHandle * passthroughNoTransformFragShader;

        ShaderModuleHandle * ptvShader;
        ShaderModuleHandle * pfShader;

        ShaderModuleHandle * uivShader;
        ShaderModuleHandle * uifShader;

        VertexInputStateHandle * ptInputState;

        VertexInputStateHandle * uiInputState;
        PipelineHandle * uiPipeline;

        renderer->CreateResources([&](ResourceCreationContext & ctx) {
            /*
                    Create quad resources
            */
            quadEbo = ctx.CreateBuffer(
                {6 * sizeof(uint32_t),
                 BufferUsageFlags::INDEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                 DEVICE_LOCAL_BIT});
            quadVbo = ctx.CreateBuffer(
                {// 4 verts, 8 floats (vec3 pos, vec3 color, vec2 texcoord)
                 8 * 4 * sizeof(float),
                 BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                 DEVICE_LOCAL_BIT});
            ctx.BufferSubData(quadEbo, (uint8_t *)&plainQuadElems[0], 0,
                              plainQuadElems.size() * sizeof(uint32_t));

            ctx.BufferSubData(quadVbo, (uint8_t *)&plainQuadVerts[0], 0,
                              plainQuadVerts.size() * sizeof(float));

            /*
                    Create passthrough shaders
            */
            ptvShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                 shaders_passthrough_transform_vert_spv_len,
                 shaders_passthrough_transform_vert_spv});

            pfShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                 shaders_passthrough_transform_frag_spv_len,
                 shaders_passthrough_transform_frag_spv});

            /*
                    Create UI shaders
            */
            uivShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                 shaders_ui_vert_spv_len, shaders_ui_vert_spv});

            uifShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                 shaders_ui_frag_spv_len, shaders_ui_frag_spv});

            /*
                    Create main renderpass
            */
            RenderPassHandle::AttachmentDescription attachment = {
                0,
                renderer->GetBackbufferFormat(),
                RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
                RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                ImageLayout::UNDEFINED,
                ImageLayout::PRESENT_SRC_KHR};

            RenderPassHandle::AttachmentReference reference = {
                0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

            RenderPassHandle::SubpassDescription subpass = {
                RenderPassHandle::PipelineBindPoint::GRAPHICS,
                0,
                nullptr,
                1,
                &reference,
                nullptr,
                nullptr,
                0,
                nullptr};

            ResourceCreationContext::RenderPassCreateInfo passInfo = {1, &attachment, 1, &subpass,
                                                                      0, nullptr};
            mainRenderpass = ctx.CreateRenderPass(passInfo);

            // Create FrameInfo
            for (size_t i = 0; i < frameInfo.size(); ++i) {
                frameInfo[i].commandBufferAllocator = ctx.CreateCommandBufferAllocator();
                frameInfo[i].canStartFrame = ctx.CreateFence(true);
                frameInfo[i].framebufferReady = ctx.CreateSemaphore();
                frameInfo[i].preRenderPassFinished = ctx.CreateSemaphore();
                frameInfo[i].mainRenderPassFinished = ctx.CreateSemaphore();
                frameInfo[i].postprocessFinished = ctx.CreateSemaphore();
            }

            /*
                    Create passthrough shader program
            */
            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding
                cameraUniformBindings[1] = {{0, DescriptorType::UNIFORM_BUFFER,
                                             ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};

            cameraPtLayout = ctx.CreateDescriptorSetLayout({1, cameraUniformBindings});

            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding
                spriteUniformBindings[2] = {{0, DescriptorType::UNIFORM_BUFFER,
                                             ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
                                            {1, DescriptorType::COMBINED_IMAGE_SAMPLER,
                                             ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

            spritePtLayout = ctx.CreateDescriptorSetLayout({2, spriteUniformBindings});

            ptPipelineLayout = ctx.CreatePipelineLayout({{cameraPtLayout, spritePtLayout}});

            ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo
                stages[2] = {{ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, ptvShader,
                              "Passthrough-Transform Vertex Shader"},
                             {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, pfShader,
                              "Passthrough-Transform Fragment Shader"}};

            ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription binding =
                {0, 8 * sizeof(float)};

            ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription
                attributes[3] = {{0, 0, VertexComponentType::FLOAT, 3, false, 0},
                                 {0, 1, VertexComponentType::FLOAT, 3, false, 3 * sizeof(float)},
                                 {0, 2, VertexComponentType::FLOAT, 2, false, 6 * sizeof(float)}};

            ResourceCreationContext::GraphicsPipelineCreateInfo::
                PipelineRasterizationStateCreateInfo ptRasterization{CullMode::BACK,
                                                                     FrontFace::CLOCKWISE};

            ptInputState = ctx.CreateVertexInputState({1, &binding, 3, attributes});

            passthroughTransformPipeline = ctx.CreateGraphicsPipeline(
                {2, stages, ptInputState, &ptRasterization, ptPipelineLayout, mainRenderpass, 0});

            // Create postprocess pipeline
            ResourceCreationContext::SamplerCreateInfo postprocessSamplerCreateInfo;
            postprocessSamplerCreateInfo.addressModeU = AddressMode::CLAMP_TO_EDGE;
            postprocessSamplerCreateInfo.addressModeV = AddressMode::CLAMP_TO_EDGE;
            postprocessSamplerCreateInfo.magFilter = Filter::NEAREST;
            postprocessSamplerCreateInfo.minFilter = Filter::NEAREST;
            postprocessSampler = ctx.CreateSampler(postprocessSamplerCreateInfo);

            RenderPassHandle::AttachmentDescription postprocessAttachment = {
                0,
                renderer->GetBackbufferFormat(),
				// TODO: Can probably be DONT_CARE once we're doing actual post processing
                RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
                RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                ImageLayout::UNDEFINED,
                ImageLayout::PRESENT_SRC_KHR};

            RenderPassHandle::AttachmentReference postprocessReference = {
                0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

            RenderPassHandle::SubpassDescription postprocessSubpass = {
                RenderPassHandle::PipelineBindPoint::GRAPHICS,
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
            postprocessRenderpass = ctx.CreateRenderPass(postprocessPassInfo);

            passthroughNoTransformVertShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                 shaders_passthrough_vert_spv_len, shaders_passthrough_vert_spv});

            passthroughNoTransformFragShader = ctx.CreateShaderModule(
                {ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                 shaders_passthrough_frag_spv_len, shaders_passthrough_frag_spv});

            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding postprocessBindings[1] =
                {{0, DescriptorType::COMBINED_IMAGE_SAMPLER,
                  ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

            postprocessDescriptorSetLayout =
                ctx.CreateDescriptorSetLayout({1, postprocessBindings});

            postprocessLayout = ctx.CreatePipelineLayout({{postprocessDescriptorSetLayout}});

            ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo
                postprocessStages[2] = {
                    {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, passthroughNoTransformVertShader,
                     "Passthrough-No Transform Vertex Shader"},
                    {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                     passthroughNoTransformFragShader, "Passthrough-No Transform Fragment Shader"}};

            postprocessPipeline =
                ctx.CreateGraphicsPipeline({2, postprocessStages, ptInputState, &ptRasterization,
                                            postprocessLayout, postprocessRenderpass, 0});

            /*
                    Create UI shader program
            */
            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uiBindings[1] = {
                {0, DescriptorType::COMBINED_IMAGE_SAMPLER,
                 ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

            uiPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({1, uiBindings});

            uiLayout = ctx.CreatePipelineLayout({{uiPipelineDescriptorSetLayout}});

            ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo
                uiStages[2] = {
                    {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, uivShader, "UI Vertex Shader"},
                    {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, uifShader,
                     "UI Fragment Shader"}};

            ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription
                uiBinding = {0, 4 * sizeof(float) + 4 * sizeof(uint8_t)};

            ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription
                uiAttributes[3] = {{0, 0, VertexComponentType::FLOAT, 2, false, 0},
                                   {0, 1, VertexComponentType::FLOAT, 2, false, 2 * sizeof(float)},
                                   {0, 2, VertexComponentType::UBYTE, 4, true, 4 * sizeof(float)}};

            ResourceCreationContext::GraphicsPipelineCreateInfo::
                PipelineRasterizationStateCreateInfo uiRasterization{CullMode::NONE,
                                                                     FrontFace::COUNTER_CLOCKWISE};

            uiInputState = ctx.CreateVertexInputState({1, &uiBinding, 3, uiAttributes});

            uiPipeline = ctx.CreateGraphicsPipeline(
                {2, uiStages, uiInputState, &uiRasterization, uiLayout, mainRenderpass, 0});

            finishedJobs++;
        });

        while (finishedJobs.load() < 1) {
            std::this_thread::sleep_for(1ms);
        }

        auto framebuffers = renderer->CreateBackbuffers(mainRenderpass);
        CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
        ctxCreateInfo.level = CommandBufferLevel::PRIMARY;
        for (size_t i = 0; i < frameInfo.size(); ++i) {
            frameInfo[i].framebuffer = framebuffers[i];
            frameInfo[i].preRenderPassCommandBuffer =
                frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
            frameInfo[i].mainCommandBuffer =
                frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
            frameInfo[i].postProcessCommandBuffer =
                frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        }

        resourceManager->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVbo);
        resourceManager->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadEbo);

        resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
        resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.frag", pfShader);

        resourceManager->AddResource("_Primitives/Shaders/ui.vert", uivShader);
        resourceManager->AddResource("_Primitives/Shaders/ui.frag", uifShader);

        resourceManager->AddResource("_Primitives/VertexInputStates/passthrough-transform.state",
                                     ptInputState);
        resourceManager->AddResource("_Primitives/Pipelines/passthrough-transform.pipe",
                                     passthroughTransformPipeline);
        resourceManager->AddResource("_Primitives/DescriptorSetLayouts/cameraPt.layout",
                                     cameraPtLayout);
        resourceManager->AddResource("_Primitives/DescriptorSetLayouts/spritePt.layout",
                                     spritePtLayout);

        resourceManager->AddResource("_Primitives/VertexInputStates/ui.state", uiInputState);
        resourceManager->AddResource("_Primitives/Pipelines/ui.pipe", uiPipeline);
        resourceManager->AddResource("_Primitives/DescriptorSetLayouts/ui.layout",
                                     uiPipelineDescriptorSetLayout);

        resourceManager->AddResource("_Primitives/PipelineLayouts/pt.pipelinelayout",
                                     ptPipelineLayout);
        resourceManager->AddResource("_Primitives/PipelineLayouts/ui.pipelinelayout", uiLayout);

        // TODO: this is ugly
        uiRenderSystem.Init(uiPipelineDescriptorSetLayout, uiPipeline, uiLayout);
    }
}

void RenderSystem::StartFrame()
{
    uiRenderSystem.StartFrame();
}

void RenderSystem::RenderFrame(SubmittedFrame const & frame)
{
    AcquireNextFrame();

    PreRenderFrame(frame);
    MainRenderFrame(frame);
    PostProcessFrame();

    auto & currFrame = frameInfo[currFrameInfoIdx];
    renderer->SwapWindow(currFrameInfoIdx, currFrame.postprocessFinished);
}

void RenderSystem::DebugOverrideBackbuffer(ImageViewHandle * image)
{
    // Drain the queue so there is no outstanding command buffer using the descriptor set
    // (Will validation layers complain here if RenderFrame is executing concurrently on a different
    // thread?)
    for (auto & fi : frameInfo) {
        fi.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
    }

    renderer->CreateResources([this, image](ResourceCreationContext & ctx) {
        if (backbufferOverride != nullptr) {
            ctx.DestroyDescriptorSet(backbufferOverride);
        }

        if (image == nullptr) {
            backbufferOverride = nullptr;
            return;
        }

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {
            postprocessSampler, image};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, imgDescriptor}};

        backbufferOverride =
            ctx.CreateDescriptorSet({1, descriptors, postprocessDescriptorSetLayout});
    });

    // Execute a dummy command buffer to signal that rendering can continue
    for (auto & fi : frameInfo) {
        fi.preRenderPassCommandBuffer->Reset();
        fi.preRenderPassCommandBuffer->BeginRecording(nullptr);
        fi.preRenderPassCommandBuffer->EndRecording();
        renderer->ExecuteCommandBuffer(fi.preRenderPassCommandBuffer, {}, {}, fi.canStartFrame);
    }
}

void RenderSystem::AcquireNextFrame()
{
    // We use the previous frame's framebufferReady semaphore because in theory we can't know what
    // frame we end up on after calling AcquireNext
    prevFrameInfoIdx = currFrameInfoIdx;
    currFrameInfoIdx =
        renderer->AcquireNextFrameIndex(frameInfo[prevFrameInfoIdx].framebufferReady, nullptr);
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
    currFrame.commandBufferAllocator->Reset();
}

void RenderSystem::PreRenderFrame(SubmittedFrame const & frame)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.preRenderPassCommandBuffer->Reset();
    currFrame.preRenderPassCommandBuffer->BeginRecording(nullptr);
    PreRenderCameras(frame.cameras);
    PreRenderSprites(frame.sprites);
    uiRenderSystem.PreRenderUi(currFrameInfoIdx, currFrame.preRenderPassCommandBuffer);
    currFrame.preRenderPassCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.preRenderPassCommandBuffer, {},
                                   {currFrame.preRenderPassFinished});
}

void RenderSystem::MainRenderFrame(SubmittedFrame const & frame)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.mainCommandBuffer->Reset();
    currFrame.mainCommandBuffer->BeginRecording(nullptr);

    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        mainRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 1, DEFAULT_CLEAR_VALUES};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo,
                                                    CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : frame.cameras) {
        RenderSprites(camera, frame.sprites);
    }

    currFrame.mainCommandBuffer->CmdEndRenderPass();
    currFrame.mainCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(
        currFrame.mainCommandBuffer,
        // TODO: If main renderpass doesn't render immediately to backbuffer in the future, framebufferReady
        // should be moved to postprocess submit
        {frameInfo[prevFrameInfoIdx].framebufferReady, currFrame.preRenderPassFinished},
        {currFrame.mainRenderPassFinished});
}

void RenderSystem::PostProcessFrame()
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    currFrame.postProcessCommandBuffer->Reset();
    currFrame.postProcessCommandBuffer->BeginRecording(nullptr);
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        postprocessRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 0, nullptr};
    currFrame.postProcessCommandBuffer->CmdBeginRenderPass(&beginInfo,
                                                           CommandBuffer::SubpassContents::INLINE);

    if (backbufferOverride != nullptr) {
        CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
        currFrame.postProcessCommandBuffer->CmdSetViewport(0, 1, &viewport);

        CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
        currFrame.postProcessCommandBuffer->CmdSetScissor(0, 1, &scissor);

        currFrame.postProcessCommandBuffer->CmdBindPipeline(
            RenderPassHandle::PipelineBindPoint::GRAPHICS, postprocessPipeline);

        currFrame.postProcessCommandBuffer->CmdBindIndexBuffer(quadEbo, 0,
                                                               CommandBuffer::IndexType::UINT32);
        currFrame.postProcessCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, 8 * sizeof(float));

        currFrame.postProcessCommandBuffer->CmdBindDescriptorSets(postprocessLayout, 0,
                                                                  {backbufferOverride});
        currFrame.postProcessCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }

	// Render UI here since post processing shouldn't(?) affect the UI
    uiRenderSystem.RenderUi(currFrameInfoIdx, currFrame.postProcessCommandBuffer);

    currFrame.postProcessCommandBuffer->CmdEndRenderPass();
    currFrame.postProcessCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.postProcessCommandBuffer,
                                   {currFrame.mainRenderPassFinished},
                                   {currFrame.postprocessFinished}, currFrame.canStartFrame);
}

void RenderSystem::PreRenderCameras(std::vector<SubmittedCamera> const & cameras)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    for (auto const & camera : cameras) {
        auto pv = camera.projection * camera.view;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(camera.uniforms, 0, sizeof(glm::mat4),
                                                              (uint32_t *)glm::value_ptr(pv));
    }
}

void RenderSystem::PreRenderSprites(std::vector<SubmittedSprite> const & sprites)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];

    for (auto const & sprite : sprites) {
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            sprite.uniforms, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(sprite.localToWorld));
    }
}

void RenderSystem::RenderSprites(SubmittedCamera const & camera,
                                 std::vector<SubmittedSprite> const & sprites)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 passthroughTransformPipeline);

    currFrame.mainCommandBuffer->CmdBindIndexBuffer(quadEbo, 0, CommandBuffer::IndexType::UINT32);
    currFrame.mainCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, 8 * sizeof(float));

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(ptPipelineLayout, 0, {camera.descriptorSet});

    for (auto const & sprite : sprites) {
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(ptPipelineLayout, 1,
                                                           {sprite.descriptorSet});
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }
}
