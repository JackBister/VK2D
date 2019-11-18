#include "Core/Rendering/RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>

#include "Core/Console/Console.h"
#include "Core/Logging/Logger.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("RenderSystem");

static constexpr CommandBuffer::ClearValue DEFAULT_CLEAR_VALUES[] = {
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}}};

RenderSystem::RenderSystem(Renderer * renderer)
    : renderer(renderer), frameInfo(renderer->GetSwapCount()), uiRenderSystem(renderer)
{
    Semaphore sem;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        // Create FrameInfo
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

    mainRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass");
    postprocessRenderpass = ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass");

    passthroughTransformPipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout");
    passthroughTransformPipeline =
        ResourceManager::GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe");

    postprocessSampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/postprocess.sampler");
    postprocessDescriptorSetLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/postprocess.layout");
    postprocessLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout");
    postprocessPipeline = ResourceManager::GetResource<PipelineHandle>("_Primitives/Pipelines/postprocess.pipe");

    quadEbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");
    quadVbo = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");

    auto framebuffers = renderer->CreateBackbuffers(mainRenderpass);
    CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
    ctxCreateInfo.level = CommandBufferLevel::PRIMARY;
    for (size_t i = 0; i < frameInfo.size(); ++i) {
        frameInfo[i].framebuffer = framebuffers[i];
        frameInfo[i].preRenderPassCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        frameInfo[i].mainCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
        frameInfo[i].postProcessCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
    }

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
}

void RenderSystem::StartFrame()
{
    for (auto & sc : scheduledDestroyers) {
        if (sc.remainingFrames == 0) {
            renderer->CreateResources(sc.fun);
        }
        --sc.remainingFrames;
    }
    scheduledDestroyers.erase(std::remove_if(scheduledDestroyers.begin(),
                                             scheduledDestroyers.end(),
                                             [](auto sc) { return sc.remainingFrames < 0; }),
                              scheduledDestroyers.end());

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

void RenderSystem::CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    renderer->CreateResources(fun);
}

void RenderSystem::DestroyResources(std::function<void(ResourceCreationContext &)> fun)
{
    // We wait for one "cycle" of frames before destroying. This should ensure that there is no rendering operation in
    // flight that is still using the resource.
    scheduledDestroyers.push_back({(int)frameInfo.size(), fun});
}

glm::ivec2 RenderSystem::GetResolution()
{
    return renderer->GetResolution();
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

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {postprocessSampler, image};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, imgDescriptor}};

        backbufferOverride = ctx.CreateDescriptorSet({1, descriptors, postprocessDescriptorSetLayout});
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
    currFrameInfoIdx = renderer->AcquireNextFrameIndex(frameInfo[prevFrameInfoIdx].framebufferReady, nullptr);
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
    renderer->ExecuteCommandBuffer(currFrame.preRenderPassCommandBuffer, {}, {currFrame.preRenderPassFinished});
}

void RenderSystem::MainRenderFrame(SubmittedFrame const & frame)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.mainCommandBuffer->Reset();
    currFrame.mainCommandBuffer->BeginRecording(nullptr);

    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        mainRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 1, DEFAULT_CLEAR_VALUES};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : frame.cameras) {
        RenderSprites(camera, frame.sprites);
    }

    currFrame.mainCommandBuffer->CmdEndRenderPass();
    currFrame.mainCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.mainCommandBuffer,
                                   // TODO: If main renderpass doesn't render immediately to backbuffer in the future,
                                   // framebufferReady should be moved to postprocess submit
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
    currFrame.postProcessCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);

    if (backbufferOverride != nullptr) {
        CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
        currFrame.postProcessCommandBuffer->CmdSetViewport(0, 1, &viewport);

        CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
        currFrame.postProcessCommandBuffer->CmdSetScissor(0, 1, &scissor);

        currFrame.postProcessCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                            postprocessPipeline);

        currFrame.postProcessCommandBuffer->CmdBindIndexBuffer(quadEbo, 0, CommandBuffer::IndexType::UINT32);
        currFrame.postProcessCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, 8 * sizeof(float));

        currFrame.postProcessCommandBuffer->CmdBindDescriptorSets(postprocessLayout, 0, {backbufferOverride});
        currFrame.postProcessCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }

    // Render UI here since post processing shouldn't(?) affect the UI
    uiRenderSystem.RenderUi(currFrameInfoIdx, currFrame.postProcessCommandBuffer);

    currFrame.postProcessCommandBuffer->CmdEndRenderPass();
    currFrame.postProcessCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.postProcessCommandBuffer,
                                   {currFrame.mainRenderPassFinished},
                                   {currFrame.postprocessFinished},
                                   currFrame.canStartFrame);
}

void RenderSystem::PreRenderCameras(std::vector<SubmittedCamera> const & cameras)
{
    auto & currFrame = frameInfo[currFrameInfoIdx];
    for (auto const & camera : cameras) {
        auto pv = camera.projection * camera.view;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            camera.uniforms, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(pv));
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

void RenderSystem::RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites)
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

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(passthroughTransformPipelineLayout, 0, {camera.descriptorSet});

    for (auto const & sprite : sprites) {
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(
            passthroughTransformPipelineLayout, 1, {sprite.descriptorSet});
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }
}
