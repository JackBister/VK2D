#include "Core/Rendering/RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>
#include <optick/optick.h>

#include "Core/Logging/Logger.h"
#include "Core/Resources/ShaderProgram.h"
#include "Core/Semaphore.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("RenderSystem");

static constexpr CommandBuffer::ClearValue DEFAULT_CLEAR_VALUES[] = {
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}}};

RenderSystem * RenderSystem::instance = nullptr;

RenderSystem * RenderSystem::GetInstance()
{
    return RenderSystem::instance;
}

void RenderSystem::StartFrame()
{
    OPTICK_EVENT();
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

    if (queuedConfigUpdate.has_value()) {
        logger->Infof("UpdateConfig");
        renderer->UpdateConfig(queuedConfigUpdate.value());
        queuedConfigUpdate.reset();
    }
    auto nextFrame = AcquireNextFrame();
    while (nextFrame == UINT32_MAX) {
        for (auto & fi : frameInfo) {
            fi.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
        }
        renderer->RecreateSwapchain();
        InitSwapchainResources();
        nextFrame = AcquireNextFrame();
    }
}

void RenderSystem::RenderFrame(SubmittedFrame const & frame)
{
    OPTICK_EVENT();

    MainRenderFrame(frame);
    PostProcessFrame();

    SubmitSwap();
}

void RenderSystem::CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    OPTICK_EVENT();
    renderer->CreateResources(fun);
}

void RenderSystem::DestroyResources(std::function<void(ResourceCreationContext &)> fun)
{
    OPTICK_EVENT();
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

uint32_t RenderSystem::AcquireNextFrame()
{
    OPTICK_EVENT();
    auto nextFrameInfoIdx = renderer->AcquireNextFrameIndex(frameInfo[currFrameInfoIdx].framebufferReady, nullptr);
    if (nextFrameInfoIdx == UINT32_MAX) {
        return nextFrameInfoIdx;
    }
    // We use the previous frame's framebufferReady semaphore because in theory we can't know what
    // frame we end up on after calling AcquireNext
    prevFrameInfoIdx = currFrameInfoIdx;
    currFrameInfoIdx = nextFrameInfoIdx;
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
    currFrame.commandBufferAllocator->Reset();
    return currFrameInfoIdx;
}

void RenderSystem::PreRenderFrame(PreRenderCommands commands)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.preRenderPassCommandBuffer->Reset();
    currFrame.preRenderPassCommandBuffer->BeginRecording(nullptr);

    PreRenderCameras(commands.cameraUpdates);
    PreRenderSprites(commands.spriteUpdates);
    PreRenderMeshes(commands.staticMeshUpdates);

    uiRenderSystem.PreRenderUi(currFrameInfoIdx, currFrame.preRenderPassCommandBuffer);
    currFrame.preRenderPassCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.preRenderPassCommandBuffer, {}, {currFrame.preRenderPassFinished});
}

void RenderSystem::MainRenderFrame(SubmittedFrame const & frame)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.mainCommandBuffer->Reset();
    currFrame.mainCommandBuffer->BeginRecording(nullptr);

    Prepass(frame);

    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        mainRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 1, DEFAULT_CLEAR_VALUES};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : frame.cameras) {
        RenderMeshes(camera, frame.meshes);
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
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    currFrame.postProcessCommandBuffer->Reset();
    currFrame.postProcessCommandBuffer->BeginRecording(nullptr);
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        postprocessRenderpass, currFrame.postprocessFramebuffer, {{0, 0}, {res.x, res.y}}, 0, nullptr};
    currFrame.postProcessCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);

    if (backbufferOverride != nullptr) {
        CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
        currFrame.postProcessCommandBuffer->CmdSetViewport(0, 1, &viewport);

        CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
        currFrame.postProcessCommandBuffer->CmdSetScissor(0, 1, &scissor);

        currFrame.postProcessCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                            postprocessProgram->GetPipeline());

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

void RenderSystem::SubmitSwap()
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    renderer->SwapWindow(currFrameInfoIdx, currFrame.postprocessFinished);
}

void RenderSystem::Prepass(SubmittedFrame const & frame)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    CommandBuffer::ClearValue depthClear;
    depthClear.type = CommandBuffer::ClearValue::Type::DEPTH_STENCIL;
    depthClear.depthStencil.depth = 1.f;
    depthClear.depthStencil.stencil = 0;
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        prepass, currFrame.prepassFramebuffer, {{0, 0}, {res.x, res.y}}, 1, &depthClear};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 prepassProgram->GetPipeline());

    for (auto const & camera : frame.cameras) {
        auto cam = GetCamera(camera.cameraHandle);
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(prepassPipelineLayout, 0, {cam->descriptorSet});

        for (auto const & mesh : frame.meshes) {
            auto meshResources = GetStaticMeshInstance(mesh.staticMeshInstance);
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(
                prepassPipelineLayout, 1, {meshResources->descriptorSet});

            for (auto const & submesh : mesh.submeshes) {
                if (submesh.indexBuffer) {
                    currFrame.mainCommandBuffer->CmdBindIndexBuffer(
                        submesh.indexBuffer, 0, CommandBuffer::IndexType::UINT32);
                    currFrame.mainCommandBuffer->CmdBindVertexBuffer(submesh.vertexBuffer, 0, 0, 8 * sizeof(float));
                    currFrame.mainCommandBuffer->CmdDrawIndexed(submesh.numIndexes, 1, 0, 0);
                } else {
                    currFrame.mainCommandBuffer->CmdBindVertexBuffer(submesh.vertexBuffer, 0, 0, 8 * sizeof(float));
                    currFrame.mainCommandBuffer->CmdDraw(submesh.numVertices, 1, 0, 0);
                }
            }
        }
    }

    currFrame.mainCommandBuffer->CmdEndRenderPass();
}

void RenderSystem::PreRenderCameras(std::vector<UpdateCamera> const & cameras)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    for (auto const & camera : cameras) {
        auto cam = GetCamera(camera.cameraHandle);
        auto pv = camera.projection * camera.view;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            cam->uniformBuffer, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(pv));
    }
}

void RenderSystem::PreRenderMeshes(std::vector<UpdateStaticMeshInstance> const & meshes)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];

    for (auto const & mesh : meshes) {
        auto meshResources = GetStaticMeshInstance(mesh.staticMeshInstance);
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            meshResources->uniformBuffer, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(mesh.localToWorld));
    }
}

void RenderSystem::RenderMeshes(SubmittedCamera const & camera, std::vector<SubmittedMesh> const & meshes)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 meshProgram->GetPipeline());

    auto cam = GetCamera(camera.cameraHandle);
    currFrame.mainCommandBuffer->CmdBindDescriptorSets(meshPipelineLayout, 0, {cam->descriptorSet});

    for (auto const & mesh : meshes) {
        auto meshResources = GetStaticMeshInstance(mesh.staticMeshInstance);
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(meshPipelineLayout, 1, {meshResources->descriptorSet});

        for (auto const & submesh : mesh.submeshes) {
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(meshPipelineLayout, 2, {submesh.materialDescriptor});
            if (submesh.indexBuffer) {
                currFrame.mainCommandBuffer->CmdBindIndexBuffer(
                    submesh.indexBuffer, 0, CommandBuffer::IndexType::UINT32);
                currFrame.mainCommandBuffer->CmdBindVertexBuffer(submesh.vertexBuffer, 0, 0, 8 * sizeof(float));
                currFrame.mainCommandBuffer->CmdDrawIndexed(submesh.numIndexes, 1, 0, 0);
            } else {
                currFrame.mainCommandBuffer->CmdBindVertexBuffer(submesh.vertexBuffer, 0, 0, 8 * sizeof(float));
                currFrame.mainCommandBuffer->CmdDraw(submesh.numVertices, 1, 0, 0);
            }
        }
    }
}

void RenderSystem::PreRenderSprites(std::vector<UpdateSpriteInstance> const & sprites)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];

    for (auto const & sprite : sprites) {
        auto spriteInstance = GetSpriteInstance(sprite.spriteInstance);
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            spriteInstance->uniformBuffer, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(sprite.localToWorld));
    }
}

void RenderSystem::RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 passthroughTransformProgram->GetPipeline());

    currFrame.mainCommandBuffer->CmdBindIndexBuffer(quadEbo, 0, CommandBuffer::IndexType::UINT32);
    currFrame.mainCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, 8 * sizeof(float));

    auto cam = GetCamera(camera.cameraHandle);
    currFrame.mainCommandBuffer->CmdBindDescriptorSets(passthroughTransformPipelineLayout, 0, {cam->descriptorSet});

    for (auto const & sprite : sprites) {
        auto spriteInstance = GetSpriteInstance(sprite.spriteInstance);
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(
            passthroughTransformPipelineLayout, 1, {spriteInstance->descriptorSet});
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }
}
