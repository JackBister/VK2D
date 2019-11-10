#include "Core/Rendering/RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>

#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/ui.frag.spv.h"
#include "Core/Rendering/Shaders/ui.vert.spv.h"
#include "Core/entity.h"
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

        DescriptorSetLayoutHandle * ptPipelineDescriptorSetLayout;

        DescriptorSetLayoutHandle * uiPipelineDescriptorSetLayout;

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
                 shaders_passthrough_frag_spv_len, shaders_passthrough_frag_spv});

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

            ResourceCreationContext::RenderPassCreateInfo passInfo = {
				1,
				&attachment,
				1,
				&subpass,
				0,
				nullptr
			};

            // Create FrameInfo
            for (size_t i = 0; i < frameInfo.size(); ++i) {
                frameInfo[i].mainRenderPass = ctx.CreateRenderPass(passInfo);
                frameInfo[i].commandBufferAllocator = ctx.CreateCommandBufferAllocator();
                frameInfo[i].canStartFrame = ctx.CreateFence(true);
                frameInfo[i].framebufferReady = ctx.CreateSemaphore();
                frameInfo[i].preRenderPassFinished = ctx.CreateSemaphore();
                frameInfo[i].mainRenderPassFinished = ctx.CreateSemaphore();
            }

            /*
                    Create passthrough shader program
            */
            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uniformBindings[2] = {
                {0, DescriptorType::UNIFORM_BUFFER,
                 ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT |
                     ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
                {1, DescriptorType::COMBINED_IMAGE_SAMPLER,
                 ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

            // DescriptorSetLayout
            ptPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({2, uniformBindings});

            ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo
                stages[2] = {{ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, ptvShader,
                              "Passthrough Vertex Shader"},
                             {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, pfShader,
                              "Passthrough Fragment Shader"}};

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
                {2, stages, ptInputState, &ptRasterization, ptPipelineDescriptorSetLayout,
                 frameInfo[0].mainRenderPass, 0});

            /*
                    Create UI shader program
            */
            ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uiBindings[1] = {
                {0, DescriptorType::COMBINED_IMAGE_SAMPLER,
                 ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

            uiPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({1, uiBindings});

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

            uiPipeline = ctx.CreateGraphicsPipeline({2, uiStages, uiInputState, &uiRasterization,
                                                     uiPipelineDescriptorSetLayout,
                                                     frameInfo[0].mainRenderPass, 0});

            finishedJobs++;
        });

        while (finishedJobs.load() < 1) {
            std::this_thread::sleep_for(1ms);
        }

        auto framebuffers = renderer->CreateBackbuffers(frameInfo[0].mainRenderPass);
        CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
        ctxCreateInfo.level = CommandBufferLevel::PRIMARY;
        for (size_t i = 0; i < frameInfo.size(); ++i) {
            frameInfo[i].framebuffer = framebuffers[i];
            frameInfo[i].mainCommandBuffer =
                frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
            frameInfo[i].preRenderPassCommandBuffer =
                frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        }

        resourceManager->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVbo);
        resourceManager->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadEbo);

        resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
        resourceManager->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

        resourceManager->AddResource("_Primitives/Shaders/ui.vert", uivShader);
        resourceManager->AddResource("_Primitives/Shaders/ui.frag", uifShader);

        resourceManager->AddResource("_Primitives/VertexInputStates/passthrough-transform.state",
                                     ptInputState);
        resourceManager->AddResource("_Primitives/Pipelines/passthrough-transform.pipe",
                                     passthroughTransformPipeline);
        resourceManager->AddResource(
            "_Primitives/DescriptorSetLayouts/passthrough-transform.layout",
            ptPipelineDescriptorSetLayout);

        resourceManager->AddResource("_Primitives/VertexInputStates/ui.state", uiInputState);
        resourceManager->AddResource("_Primitives/Pipelines/ui.pipe", uiPipeline);
        resourceManager->AddResource("_Primitives/DescriptorSetLayouts/ui.layout",
                                     uiPipelineDescriptorSetLayout);

        // TODO: this is ugly
        uiRenderSystem.Init(uiPipelineDescriptorSetLayout, uiPipeline);
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

    auto & currFrame = frameInfo[currFrameInfoIdx];
    renderer->SwapWindow(currFrameInfoIdx, currFrame.mainRenderPassFinished);
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
    for (auto const & camera : frame.cameras) {
        PreRenderSprites(camera, frame.sprites);
    }
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

    CommandBuffer::ClearValue clearValues[] = {
        {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}}};
    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        currFrame.mainRenderPass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 1, clearValues};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo,
                                                    CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : frame.cameras) {
        RenderSprites(camera, frame.sprites);
    }

    uiRenderSystem.RenderUi(currFrameInfoIdx, currFrame.mainCommandBuffer);

    currFrame.mainCommandBuffer->CmdEndRenderPass();
    currFrame.mainCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(
        currFrame.mainCommandBuffer,
        {frameInfo[prevFrameInfoIdx].framebufferReady, currFrame.preRenderPassFinished},
        {currFrame.mainRenderPassFinished}, currFrame.canStartFrame);
}

void RenderSystem::PreRenderSprites(SubmittedCamera const & camera,
                                    std::vector<SubmittedSprite> const & sprites)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    // TODO: Broken for scenes with multiple cameras
    for (auto const & sprite : sprites) {
        auto pvm = camera.projection * camera.view * sprite.localToWorld;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(sprite.uniforms, 0, sizeof(glm::mat4),
                                                              (uint32_t *)glm::value_ptr(pvm));
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

    for (auto const & sprite : sprites) {
        currFrame.mainCommandBuffer->CmdBindDescriptorSet(sprite.descriptorSet);
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }
}
