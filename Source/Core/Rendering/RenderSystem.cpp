#include "Core/Rendering/RenderSystem.h"

#include <set>

#include <glm/gtc/type_ptr.hpp>
#include <optick/optick.h>

#include "Core/Logging/Logger.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ShaderProgram.h"
#include "Core/Resources/SkeletalMesh.h"
#include "Core/Resources/StaticMesh.h"
#include "Core/Semaphore.h"
#include "Core/entity.h"
#include "DebugDrawSystem.h"
#include "Vertex.h"

static const auto logger = Logger::Create("RenderSystem");

static constexpr CommandBuffer::ClearValue DEFAULT_CLEAR_VALUES[] = {
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}},
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 0.f, 0.f}},
};

static constexpr size_t MIN_INDEXES_BUFFER_SIZE = 128 * sizeof(uint32_t);

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

void RenderSystem::RenderFrame()
{
    OPTICK_EVENT();

    MainRenderFrame();
    PostProcessFrame();

    SubmitSwap();
}

void RenderSystem::CreateResources(std::function<void(ResourceCreationContext &)> && fun)
{
    OPTICK_EVENT();
    renderer->CreateResources(fun);
}

void RenderSystem::DestroyResources(std::function<void(ResourceCreationContext &)> && fun)
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
    PreRenderLights(commands.lightUpdates);
    PreRenderSkeletalMeshes(commands.skeletalMeshUpdates);
    PreRenderSprites(commands.spriteUpdates);
    PreRenderMeshes(commands.staticMeshUpdates);

    UpdateAnimations();
    UpdateLights();

    uiRenderSystem.PreRenderUi(currFrameInfoIdx, currFrame.preRenderPassCommandBuffer);
    currFrame.preRenderPassCommandBuffer->EndRecording();
    renderer->ExecuteCommandBuffer(currFrame.preRenderPassCommandBuffer, {}, {currFrame.preRenderPassFinished});
}

void RenderSystem::MainRenderFrame()
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    currFrame.mainCommandBuffer->Reset();
    currFrame.mainCommandBuffer->BeginRecording(nullptr);

    auto meshBatches = CreateBatches();

    Prepass(meshBatches);

    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        mainRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 2, DEFAULT_CLEAR_VALUES};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : cameras) {
        if (!camera.isActive) {
            continue;
        }
        RenderMeshes(camera, meshBatches);
        RenderSprites(camera);

        RenderTransparentMeshes(camera, meshBatches);
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
        currFrame.postProcessCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, sizeof(VertexWithColorAndUv));

        currFrame.postProcessCommandBuffer->CmdBindDescriptorSets(postprocessLayout, 0, {backbufferOverride});
        currFrame.postProcessCommandBuffer->CmdDrawIndexed(6, 1, 0, 0, 0);
    }

    for (auto const & camera : cameras) {
        if (!camera.isActive) {
            continue;
        }
        RenderDebugDraws(camera);
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

void RenderSystem::Prepass(std::vector<MeshBatch> const & batches)
{
    OPTICK_EVENT();
    if (batches.size() == 0) {
        return;
    }
    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    CommandBuffer::ClearValue depthClear;
    depthClear.type = CommandBuffer::ClearValue::Type::DEPTH_STENCIL;
    depthClear.depthStencil.depth = 1.f;
    depthClear.depthStencil.stencil = 0;
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        prepass, currFrame.prepassFramebuffer, {{0, 0}, {res.x, res.y}}, 1, &depthClear};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);

    if (!currFrame.meshUniformsDescriptorSet) {
        // If there are no meshes in the scene, the prepass is not used.
        // TODO: We begin and end the renderpass just to transition the prepass depth image into the right layout.
        // This is slightly wasteful but I currently think it's more work than it's worth to create alternate
        // framebuffers and render passes for sprite-only scenes
        currFrame.mainCommandBuffer->CmdEndRenderPass();
        return;
    }

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    // currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
    //                                             prepassProgram->GetPipeline());

    size_t currentBoneOffset = 0;
    DescriptorSet * currentBoneSet = nullptr;
    ShaderProgram * currentShaderProgram = nullptr;
    for (auto const & cam : cameras) {
        if (!cam.isActive) {
            continue;
        }
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(
            prepassPipelineLayout, 0, {cam.descriptorSet, currFrame.meshUniformsDescriptorSet});
        DescriptorSet * currentMeshDescriptorSet = nullptr;
        for (auto const & batch : batches) {
            if (!batch.material->GetDescriptorSet()) {
                continue;
            }
            if (batch.material->GetAlbedo()->HasTransparency()) {
                continue;
            }

            if (batch.shaderProgram != currentShaderProgram) {
                currentShaderProgram = batch.shaderProgram;
                currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                             batch.shaderProgram == meshProgram
                                                                 ? prepassProgram->GetPipeline()
                                                                 : skeletalPrepassProgram->GetPipeline());
            }
            if (batch.shaderProgram == skeletalMeshProgram &&
                (batch.boneTransformsOffset != currentBoneOffset || !currentBoneSet)) {
                currentBoneOffset = batch.boneTransformsOffset;
                currentBoneSet = currFrame.boneTransformOffsets[batch.boneTransformsOffset];
                currFrame.mainCommandBuffer->CmdBindDescriptorSets(skeletalPrepassPipelineLayout, 2, {currentBoneSet});
            }

            currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, batch.vertexSize);
            if (batch.indexBuffer != nullptr) {
                currFrame.mainCommandBuffer->CmdBindIndexBuffer(batch.indexBuffer, 0, CommandBuffer::IndexType::UINT32);
            }

            if (batch.drawCommandsCount > 0) {
                currFrame.mainCommandBuffer->CmdDrawIndirect(currFrame.meshIndirect,
                                                             batch.drawCommandsOffset * sizeof(DrawIndirectCommand),
                                                             batch.drawCommandsCount);
            }
            if (batch.drawIndexedCommands.size() > 0) {
                for (auto const & command : batch.drawIndexedCommands) {
                    currFrame.mainCommandBuffer->CmdDrawIndexed(command.indexCount,
                                                                command.instanceCount,
                                                                command.firstIndex,
                                                                command.vertexOffset,
                                                                command.firstInstance);
                }
            }
        }
    }

    currFrame.mainCommandBuffer->CmdEndRenderPass();
}

void RenderSystem::PreRenderCameras(std::vector<UpdateCamera> const & cameraUpdates)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];
    for (auto const & camera : cameraUpdates) {
        auto cam = GetCamera(camera.cameraHandle);
        cam->isActive = camera.isActive;
        struct {
            glm::mat4 pv;
            glm::vec3 pos;
        } uniforms = {camera.projection * camera.view, camera.pos};
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            cam->uniformBuffer, 0, sizeof(glm::mat4) + sizeof(glm::vec3), (uint32_t *)&uniforms);
    }
}

void RenderSystem::PreRenderMeshes(std::vector<UpdateStaticMeshInstance> const & meshes)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];

    for (auto const & meshUpdate : meshes) {
        auto instance = GetStaticMeshInstance(meshUpdate.staticMeshInstance);
        instance->isActive = meshUpdate.isActive;
        instance->localToWorld = meshUpdate.localToWorld;
    }
}

void RenderSystem::RenderMeshes(CameraInstance const & cam, std::vector<MeshBatch> const & batches)
{
    OPTICK_EVENT();
    if (batches.size() == 0) {
        return;
    }
    auto & currFrame = frameInfo[currFrameInfoIdx];

    if (!currFrame.meshUniformsDescriptorSet) {
        return;
    }

    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    //    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
    //                                               meshProgram->GetPipeline());

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(
        meshPipelineLayout, 0, {currFrame.lightsDescriptorSet, cam.descriptorSet, currFrame.meshUniformsDescriptorSet});

    DescriptorSet * currentMaterialDescriptorSet = nullptr;
    size_t currentBoneOffset = 0;
    DescriptorSet * currentBoneSet = nullptr;
    ShaderProgram * currentShaderProgram = nullptr;
    for (auto const & batch : batches) {
        if (!batch.material->GetDescriptorSet()) {
            continue;
        }
        if (batch.material->GetAlbedo()->HasTransparency()) {
            continue;
        }

        if (batch.material->GetDescriptorSet() != currentMaterialDescriptorSet) {
            currentMaterialDescriptorSet = batch.material->GetDescriptorSet();
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(
                meshPipelineLayout, 3, {batch.material->GetDescriptorSet()});
        }

        if (batch.shaderProgram != currentShaderProgram) {
            currentShaderProgram = batch.shaderProgram;
            currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                         batch.shaderProgram->GetPipeline());
        }

        if (batch.shaderProgram == skeletalMeshProgram &&
            (batch.boneTransformsOffset != currentBoneOffset || !currentBoneSet)) {
            currentBoneOffset = batch.boneTransformsOffset;
            currentBoneSet = currFrame.boneTransformOffsets[batch.boneTransformsOffset];
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(skeletalMeshPipelineLayout, 4, {currentBoneSet});
        }

        currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, batch.vertexSize);
        if (batch.indexBuffer != nullptr) {
            currFrame.mainCommandBuffer->CmdBindIndexBuffer(batch.indexBuffer, 0, CommandBuffer::IndexType::UINT32);
        }
        if (batch.drawCommandsCount > 0) {
            currFrame.mainCommandBuffer->CmdDrawIndirect(currFrame.meshIndirect,
                                                         batch.drawCommandsOffset * sizeof(DrawIndirectCommand),
                                                         batch.drawCommandsCount);
        }
        if (batch.drawIndexedCommands.size() > 0) {
            for (auto const & command : batch.drawIndexedCommands) {
                currFrame.mainCommandBuffer->CmdDrawIndexed(command.indexCount,
                                                            command.instanceCount,
                                                            command.firstIndex,
                                                            command.vertexOffset,
                                                            command.firstInstance);
            }
        }
    }
}

void RenderSystem::RenderTransparentMeshes(CameraInstance const & cam, std::vector<MeshBatch> const & batches)
{
    OPTICK_EVENT();
    if (batches.size() == 0) {
        return;
    }
    auto & currFrame = frameInfo[currFrameInfoIdx];

    if (!currFrame.meshUniformsDescriptorSet) {
        return;
    }

    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    // currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
    //                                             transparentMeshProgram->GetPipeline());

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(
        meshPipelineLayout, 0, {currFrame.lightsDescriptorSet, cam.descriptorSet, currFrame.meshUniformsDescriptorSet});

    size_t currentBoneOffset = 0;
    DescriptorSet * currentBoneSet = nullptr;
    DescriptorSet * currentMaterialDescriptorSet = nullptr;
    ShaderProgram * currentShaderProgram = nullptr;
    for (auto const & batch : batches) {
        if (!batch.material->GetDescriptorSet()) {
            continue;
        }
        if (!batch.material->GetAlbedo()->HasTransparency()) {
            continue;
        }

        if (batch.material->GetDescriptorSet() != currentMaterialDescriptorSet) {
            currentMaterialDescriptorSet = batch.material->GetDescriptorSet();
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(
                meshPipelineLayout, 3, {batch.material->GetDescriptorSet()});
        }

        if (batch.shaderProgram != currentShaderProgram) {
            currentShaderProgram = batch.shaderProgram;
            currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                         batch.shaderProgram->GetPipeline());
        }

        if (batch.shaderProgram == skeletalMeshProgram &&
            (batch.boneTransformsOffset != currentBoneOffset || !currentBoneSet)) {
            currentBoneOffset = batch.boneTransformsOffset;
            currentBoneSet = currFrame.boneTransformOffsets[batch.boneTransformsOffset];
            currFrame.mainCommandBuffer->CmdBindDescriptorSets(skeletalMeshPipelineLayout, 4, {currentBoneSet});
        }

        currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, batch.vertexSize);
        if (batch.indexBuffer != nullptr) {
            currFrame.mainCommandBuffer->CmdBindIndexBuffer(batch.indexBuffer, 0, CommandBuffer::IndexType::UINT32);
        }
        if (batch.drawCommandsCount > 0) {
            currFrame.mainCommandBuffer->CmdDrawIndirect(currFrame.meshIndirect,
                                                         batch.drawCommandsOffset * sizeof(DrawIndirectCommand),
                                                         batch.drawCommandsCount);
        }
        if (batch.drawIndexedCommands.size() > 0) {
            for (auto const & command : batch.drawIndexedCommands) {
                currFrame.mainCommandBuffer->CmdDrawIndexed(command.indexCount,
                                                            command.instanceCount,
                                                            command.firstIndex,
                                                            command.vertexOffset,
                                                            command.firstInstance);
            }
        }
    }
}

void RenderSystem::RenderDebugDraws(CameraInstance const & camera)
{
    OPTICK_EVENT();
    auto draws = DebugDrawSystem::GetInstance()->GetCurrentDraws();

    if (draws.lines.size() == 0 && draws.points.size() == 0) {
        return;
    }

    auto & currFrame = frameInfo[currFrameInfoIdx];
    auto res = renderer->GetResolution();

    size_t requiredLinesSize = draws.lines.size() * 4 * sizeof(glm::vec3);
    size_t requiredPointsSize = draws.points.size() * 2 * sizeof(glm::vec3);
    if (requiredPointsSize > currFrame.debugPointsSize || requiredLinesSize > currFrame.debugLinesSize) {
        Semaphore sem;
        CreateResources([this, &currFrame, &sem, requiredLinesSize, requiredPointsSize](ResourceCreationContext & ctx) {
            if (requiredLinesSize > currFrame.debugLinesSize) {
                if (currFrame.debugLinesMapped && currFrame.debugLines) {
                    ctx.UnmapBuffer(currFrame.debugLines);
                    ctx.DestroyBuffer(currFrame.debugLines);
                }
                currFrame.debugLinesSize = requiredLinesSize;
                ResourceCreationContext::BufferCreateInfo debugLinesCreateInfo;
                debugLinesCreateInfo.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                debugLinesCreateInfo.size = currFrame.debugLinesSize;
                debugLinesCreateInfo.usage = BufferUsageFlags::VERTEX_BUFFER_BIT;
                currFrame.debugLines = ctx.CreateBuffer(debugLinesCreateInfo);
                currFrame.debugLinesMapped =
                    (glm::vec3 *)ctx.MapBuffer(currFrame.debugLines, 0, currFrame.debugLinesSize);
            }
            if (requiredPointsSize > currFrame.debugPointsSize) {
                if (currFrame.debugPointsMapped && currFrame.debugPoints) {
                    ctx.UnmapBuffer(currFrame.debugPoints);
                    ctx.DestroyBuffer(currFrame.debugPoints);
                }
                currFrame.debugPointsSize = requiredPointsSize;
                ResourceCreationContext::BufferCreateInfo debugPointsCreateInfo;
                debugPointsCreateInfo.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                debugPointsCreateInfo.size = currFrame.debugPointsSize;
                debugPointsCreateInfo.usage = BufferUsageFlags::VERTEX_BUFFER_BIT;
                currFrame.debugPoints = ctx.CreateBuffer(debugPointsCreateInfo);
                currFrame.debugPointsMapped =
                    (glm::vec3 *)ctx.MapBuffer(currFrame.debugPoints, 0, currFrame.debugPointsSize);
            }
            sem.Signal();
        });
        sem.Wait();
    }

    size_t debugLinesIdx = 0;
    for (auto const & line : draws.lines) {
        currFrame.debugLinesMapped[debugLinesIdx++] = line.worldSpaceStartPos;
        currFrame.debugLinesMapped[debugLinesIdx++] = line.color;
        currFrame.debugLinesMapped[debugLinesIdx++] = line.worldSpaceEndPos;
        currFrame.debugLinesMapped[debugLinesIdx++] = line.color;
    }

    size_t debugPointsIdx = 0;
    for (auto const & point : draws.points) {
        currFrame.debugPointsMapped[debugPointsIdx++] = point.worldSpacePos;
        currFrame.debugPointsMapped[debugPointsIdx++] = point.color;
    }

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.postProcessCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.postProcessCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.postProcessCommandBuffer->CmdBindDescriptorSets(debugDrawLayout, 0, {camera.descriptorSet});

    if (draws.lines.size() > 0) {
        currFrame.postProcessCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                            debugDrawLinesProgram->GetPipeline());
        currFrame.postProcessCommandBuffer->CmdBindVertexBuffer(currFrame.debugLines, 0, 0, 2 * sizeof(glm::vec3));
        currFrame.postProcessCommandBuffer->CmdDraw(draws.lines.size() * 2, 1, 0, 0);
    }

    if (draws.points.size() > 0) {
        currFrame.postProcessCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                            debugDrawPointsProgram->GetPipeline());
        currFrame.postProcessCommandBuffer->CmdBindVertexBuffer(currFrame.debugPoints, 0, 0, 2 * sizeof(glm::vec3));
        currFrame.postProcessCommandBuffer->CmdDraw(draws.points.size(), 1, 0, 0);
    }
}

std::vector<MeshBatch> RenderSystem::CreateBatches()
{
    OPTICK_EVENT();

    auto & currFrame = frameInfo[currFrameInfoIdx];

    std::set<StaticMeshInstanceId> inactiveInstances;
    std::unordered_map<StaticMeshInstanceId, size_t> instanceIdToLtwIndex;
    std::vector<glm::mat4> localToWorlds;
    {
        OPTICK_EVENT("BuildLtwBuffer");
        for (auto const & mesh : skeletalMeshes) {
            if (!mesh.isActive) {
                continue;
            }
            // TODO: +1000000 is just a hack for now to get skeletal meshes rendering
            auto existingLtwIndex = instanceIdToLtwIndex.find(mesh.id + 1000000);
            if (existingLtwIndex == instanceIdToLtwIndex.end()) {
                localToWorlds.push_back(mesh.localToWorld);
                instanceIdToLtwIndex[mesh.id + 1000000] = localToWorlds.size() - 1;
            }
        }
        for (auto const & submeshInstance : sortedSubmeshInstances) {
            auto const staticMeshInstance = GetStaticMeshInstance(submeshInstance.instanceId);
            if (!staticMeshInstance->isActive) {
                inactiveInstances.insert(submeshInstance.instanceId);
                continue;
            }
            auto existingLtwIndex = instanceIdToLtwIndex.find(submeshInstance.instanceId);
            if (existingLtwIndex == instanceIdToLtwIndex.end()) {
                localToWorlds.push_back(staticMeshInstance->localToWorld);
                instanceIdToLtwIndex[submeshInstance.instanceId] = localToWorlds.size() - 1;
            }
        }
    }

    // I've probably made this part overly complicated...
    std::vector<size_t> neededBoneOffsetDescriptorSets;
    std::unordered_map<SkeletalMeshInstanceId, size_t> instanceIdToBoneOffset;
    std::vector<glm::mat4> bones;
    std::vector<size_t> boneOffsets;
    std::vector<size_t> boneSplits;
    size_t totalBoneSize = 0;
    {
        OPTICK_EVENT("BuildBoneBuffer");
        size_t currentBoneOffset = 0;
        for (auto const & mesh : skeletalMeshes) {
            if (!mesh.isActive) {
                continue;
            }

            for (auto const & bone : mesh.bones) {
                bones.push_back(bone.currentTransform);
            }
            boneOffsets.push_back(currentBoneOffset);
            boneSplits.push_back(mesh.bones.size());
            instanceIdToBoneOffset[mesh.id] = currentBoneOffset;
            if (currFrame.boneTransformOffsets.find(currentBoneOffset) == currFrame.boneTransformOffsets.end()) {
                neededBoneOffsetDescriptorSets.push_back(currentBoneOffset);
            }

            totalBoneSize += mesh.bones.size() * sizeof(glm::mat4);
            currentBoneOffset += mesh.bones.size() * sizeof(glm::mat4);
            auto uboAlignment = rendererProperties.GetUniformBufferAlignment();
            if (uboAlignment != 0 && currentBoneOffset % uboAlignment != 0) {
                // Fix alignment - the bones will be submitted as a UBO and UBO offsets must follow alignment
                size_t alignment = currentBoneOffset % uboAlignment;
                currentBoneOffset += alignment;
                totalBoneSize += alignment;
            }
        }
    }

    std::vector<MeshBatch> batches;
    size_t drawCommandsOffset = 0;
    std::vector<DrawIndirectCommand> drawCommands;
    size_t drawIndexedOffset = 0;
    std::vector<DrawIndexedIndirectCommand> drawIndexedCommands;
    {
        OPTICK_EVENT("GatherBatches");
        MeshBatch currentBatch;
        currentBatch.vertexSize = sizeof(VertexWithSkinning);
        currentBatch.shaderProgram = skeletalMeshProgram;
        // TODO: Merge with static mesh batches
        for (auto const & mesh : skeletalMeshes) {
            if (!mesh.isActive) {
                continue;
            }
            auto offset = instanceIdToBoneOffset.at(mesh.id);
            for (auto const & submesh : mesh.mesh->GetSubmeshes()) {
                if (submesh.GetMaterial() != currentBatch.material ||
                    submesh.GetVertexBuffer().GetBuffer() != currentBatch.vertexBuffer ||
                    (submesh.GetIndexBuffer().has_value() ? submesh.GetIndexBuffer().value().GetBuffer() : nullptr) !=
                        currentBatch.indexBuffer ||
                    offset != currentBatch.boneTransformsOffset) {
                    currentBatch.drawCommandsOffset = drawCommandsOffset;
                    currentBatch.drawCommandsCount = drawCommands.size() - drawCommandsOffset;
                    currentBatch.drawIndexedCommandsOffset = drawIndexedOffset;
                    currentBatch.drawIndexedCommandsCount = drawIndexedCommands.size() - drawIndexedOffset;
                    if (currentBatch.material != nullptr) {
                        batches.push_back(currentBatch);
                    }
                    drawCommandsOffset = drawCommands.size();
                    drawIndexedOffset = drawIndexedCommands.size();
                    currentBatch.drawCommands.clear();
                    currentBatch.drawIndexedCommands.clear();
                    currentBatch.indexBuffer =
                        (submesh.GetIndexBuffer().has_value() ? submesh.GetIndexBuffer().value().GetBuffer() : nullptr);
                    currentBatch.material = submesh.GetMaterial();
                    currentBatch.vertexBuffer = submesh.GetVertexBuffer().GetBuffer();
                    currentBatch.boneTransformsOffset = offset;
                }
                if (submesh.GetIndexBuffer().has_value()) {
                    DrawIndexedIndirectCommand command;
                    command.firstIndex = submesh.GetIndexBuffer().value().GetOffset() / sizeof(uint32_t);
                    command.firstInstance = instanceIdToLtwIndex.at(mesh.id + 1000000);
                    command.indexCount = submesh.GetIndexBuffer().value().GetSize() / sizeof(uint32_t);
                    command.instanceCount = 1;
                    command.vertexOffset = submesh.GetVertexBuffer().GetOffset() / sizeof(VertexWithSkinning);
                    currentBatch.drawIndexedCommands.push_back(command);
                    drawIndexedCommands.push_back(command);
                } else {
                    DrawIndirectCommand command;
                    command.firstInstance = instanceIdToLtwIndex.at(mesh.id + 1000000);
                    command.firstVertex = submesh.GetVertexBuffer().GetOffset() / sizeof(VertexWithSkinning);
                    command.instanceCount = 1;
                    command.vertexCount = submesh.GetVertexBuffer().GetSize() / sizeof(VertexWithSkinning);
                    currentBatch.drawCommands.push_back(command);
                    drawCommands.push_back(command);
                }
            }
        }

        if (currentBatch.material != nullptr) {
            batches.push_back(currentBatch);
        }
        // Force new batch for static meshes since vertex size and shader program is different
        currentBatch.material = nullptr;

        currentBatch.vertexSize = sizeof(VertexWithNormal);
        currentBatch.shaderProgram = meshProgram;
        for (auto const & submesh : sortedSubmeshInstances) {
            // TODO: Replace with contains when upgraded to C++20.
            // I can't belive a set implementation does not have a straightforward contains method...
            if (inactiveInstances.count(submesh.instanceId) > 0) {
                continue;
            }
            if (submesh.submesh->GetMaterial() != currentBatch.material ||
                submesh.submesh->GetVertexBuffer().GetBuffer() != currentBatch.vertexBuffer ||
                (submesh.submesh->GetIndexBuffer().has_value() ? submesh.submesh->GetIndexBuffer()->GetBuffer()
                                                               : nullptr) != currentBatch.indexBuffer) {
                currentBatch.drawCommandsOffset = drawCommandsOffset;
                currentBatch.drawCommandsCount = drawCommands.size() - drawCommandsOffset;
                currentBatch.drawIndexedCommandsOffset = drawIndexedOffset;
                currentBatch.drawIndexedCommandsCount = drawIndexedCommands.size() - drawIndexedOffset;
                if (currentBatch.material != nullptr) {
                    if (currentBatch.material->GetAlbedo()->HasTransparency()) {
                        currentBatch.shaderProgram = transparentMeshProgram;
                    } else {
                        currentBatch.shaderProgram = meshProgram;
                    }
                    batches.push_back(currentBatch);
                }
                drawCommandsOffset = drawCommands.size();
                drawIndexedOffset = drawIndexedCommands.size();
                currentBatch.drawCommands.clear();
                currentBatch.drawIndexedCommands.clear();
                currentBatch.indexBuffer =
                    (submesh.submesh->GetIndexBuffer().has_value() ? submesh.submesh->GetIndexBuffer()->GetBuffer()
                                                                   : nullptr);
                currentBatch.material = submesh.submesh->GetMaterial();
                currentBatch.vertexBuffer = submesh.submesh->GetVertexBuffer().GetBuffer();
            }
            if (submesh.submesh->GetIndexBuffer().has_value()) {
                DrawIndexedIndirectCommand command;
                command.firstIndex = submesh.submesh->GetIndexBuffer()->GetOffset();
                command.firstInstance = instanceIdToLtwIndex.at(submesh.instanceId);
                command.indexCount = submesh.submesh->GetNumIndexes();
                command.instanceCount = 1;
                command.vertexOffset = submesh.submesh->GetVertexBuffer().GetOffset() / sizeof(VertexWithNormal);
                currentBatch.drawIndexedCommands.push_back(command);
                drawIndexedCommands.push_back(command);
            } else {
                DrawIndirectCommand command;
                command.firstInstance = instanceIdToLtwIndex.at(submesh.instanceId);
                command.firstVertex = submesh.submesh->GetVertexBuffer().GetOffset() / sizeof(VertexWithNormal);
                command.instanceCount = 1;
                command.vertexCount = submesh.submesh->GetNumVertices();
                currentBatch.drawCommands.push_back(command);
                drawCommands.push_back(command);
            }
        }
        if (currentBatch.material != nullptr) {
            currentBatch.drawCommandsOffset = drawCommandsOffset;
            currentBatch.drawCommandsCount = drawCommands.size() - drawCommandsOffset;
            currentBatch.drawIndexedCommandsOffset = drawIndexedOffset;
            currentBatch.drawIndexedCommandsCount = drawIndexedCommands.size() - drawIndexedOffset;
            batches.push_back(currentBatch);
        }
    }

    Semaphore uniformCreationDone;
    if (localToWorlds.size() * sizeof(glm::mat4) > currFrame.meshUniformsSize ||
        drawCommands.size() * sizeof(DrawIndirectCommand) > currFrame.meshIndirectSize ||
        drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand) > currFrame.meshIndexedIndirectSize ||
        neededBoneOffsetDescriptorSets.size() > 0 || totalBoneSize > currFrame.boneTransformsSize) {
        renderer->CreateResources([this,
                                   &uniformCreationDone,
                                   &currFrame,
                                   &drawCommands,
                                   &drawIndexedCommands,
                                   &localToWorlds,
                                   &bones,
                                   totalBoneSize,
                                   &instanceIdToBoneOffset,
                                   &neededBoneOffsetDescriptorSets](ResourceCreationContext & ctx) {
            OPTICK_EVENT("CreateUniformBuffers");
            if (localToWorlds.size() * sizeof(glm::mat4) > currFrame.meshUniformsSize) {
                if (currFrame.meshUniforms) {
                    ctx.UnmapBuffer(currFrame.meshUniforms);
                    ctx.DestroyBuffer(currFrame.meshUniforms);
                }
                currFrame.meshUniformsSize = localToWorlds.size() * sizeof(glm::mat4);
                ResourceCreationContext::BufferCreateInfo uniformsCreateInfo;
                uniformsCreateInfo.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                uniformsCreateInfo.size = currFrame.meshUniformsSize;
                uniformsCreateInfo.usage = BufferUsageFlags::UNIFORM_BUFFER_BIT;
                currFrame.meshUniforms = ctx.CreateBuffer(uniformsCreateInfo);
                currFrame.meshUniformsMapped =
                    (glm::mat4 *)ctx.MapBuffer(currFrame.meshUniforms, 0, currFrame.meshUniformsSize);
            }
            if (currFrame.meshUniformsDescriptorSet) {
                ctx.DestroyDescriptorSet(currFrame.meshUniformsDescriptorSet);
            }
            {
                ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptors[1] = {
                    {currFrame.meshUniforms, 0, currFrame.meshUniformsSize}};
                ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[2] = {
                    {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptors[0]}};
                ResourceCreationContext::DescriptorSetCreateInfo uniformsDescriptorSetCi;
                uniformsDescriptorSetCi.descriptorCount = 1;
                uniformsDescriptorSetCi.descriptors = descriptors;
                uniformsDescriptorSetCi.layout = this->meshModelLayout;
                currFrame.meshUniformsDescriptorSet = ctx.CreateDescriptorSet(uniformsDescriptorSetCi);
            }

            if (drawCommands.size() * sizeof(DrawIndirectCommand) > currFrame.meshIndirectSize) {
                if (currFrame.meshIndirect) {
                    ctx.UnmapBuffer(currFrame.meshIndirect);
                    ctx.DestroyBuffer(currFrame.meshIndirect);
                }
                currFrame.meshIndirectSize = drawCommands.size() * sizeof(DrawIndirectCommand);
                if (currFrame.meshIndirectSize == 0) {
                    currFrame.meshIndirectSize = MIN_INDEXES_BUFFER_SIZE;
                }
                ResourceCreationContext::BufferCreateInfo meshIndirectCreateInfo;
                meshIndirectCreateInfo.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                meshIndirectCreateInfo.size = currFrame.meshIndirectSize;
                meshIndirectCreateInfo.usage = BufferUsageFlags::INDIRECT_BUFFER_BIT;
                currFrame.meshIndirect = ctx.CreateBuffer(meshIndirectCreateInfo);
                currFrame.meshIndirectMapped =
                    (DrawIndirectCommand *)ctx.MapBuffer(currFrame.meshIndirect, 0, currFrame.meshIndirectSize);
            }
            if (drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand) > currFrame.meshIndexedIndirectSize) {
                if (currFrame.meshIndexedIndirect) {
                    ctx.UnmapBuffer(currFrame.meshIndexedIndirect);
                    ctx.DestroyBuffer(currFrame.meshIndexedIndirect);
                }
                currFrame.meshIndexedIndirectSize = drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand);
                if (currFrame.meshIndexedIndirectSize == 0) {
                    currFrame.meshIndexedIndirectSize = MIN_INDEXES_BUFFER_SIZE;
                }
                ResourceCreationContext::BufferCreateInfo meshIndexedIndirectCreateInfo;
                meshIndexedIndirectCreateInfo.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                meshIndexedIndirectCreateInfo.size = currFrame.meshIndexedIndirectSize;
                meshIndexedIndirectCreateInfo.usage = BufferUsageFlags::INDIRECT_BUFFER_BIT;
                currFrame.meshIndexedIndirect = ctx.CreateBuffer(meshIndexedIndirectCreateInfo);
                currFrame.meshIndexedIndirectMapped = (DrawIndexedIndirectCommand *)ctx.MapBuffer(
                    currFrame.meshIndexedIndirect, 0, currFrame.meshIndexedIndirectSize);
            }

            if (totalBoneSize > currFrame.boneTransformsSize) {
                for (auto const & descriptorSet : currFrame.boneTransformOffsets) {
                    ctx.DestroyDescriptorSet(descriptorSet.second);
                }
                currFrame.boneTransformOffsets.clear();
                if (currFrame.boneTransforms) {
                    ctx.UnmapBuffer(currFrame.boneTransforms);
                    ctx.DestroyBuffer(currFrame.boneTransforms);
                }
                currFrame.boneTransformsSize = totalBoneSize;
                ResourceCreationContext::BufferCreateInfo boneTransformsCi;
                boneTransformsCi.memoryProperties =
                    MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
                boneTransformsCi.size = currFrame.boneTransformsSize;
                boneTransformsCi.usage = BufferUsageFlags::UNIFORM_BUFFER_BIT;
                currFrame.boneTransforms = ctx.CreateBuffer(boneTransformsCi);
                currFrame.boneTransformsMapped =
                    (glm::mat4 *)ctx.MapBuffer(currFrame.boneTransforms, 0, currFrame.boneTransformsSize);

                std::set<size_t> createdOffsets;
                for (auto const & offset : instanceIdToBoneOffset) {
                    if (createdOffsets.count(offset.second) > 0) {
                        continue;
                    }
                    ResourceCreationContext::DescriptorSetCreateInfo descriptorSetCi;
                    ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor bufferDescriptor;
                    bufferDescriptor.buffer = currFrame.boneTransforms;
                    bufferDescriptor.offset = offset.second;
                    bufferDescriptor.range = currFrame.boneTransformsSize - offset.second;
                    ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
                        {DescriptorType::UNIFORM_BUFFER, 0, bufferDescriptor},
                    };
                    descriptorSetCi.descriptors = descriptors;
                    descriptorSetCi.descriptorCount = 1;
                    descriptorSetCi.layout = skeletalMeshBoneLayout;
                    auto descriptorSet = ctx.CreateDescriptorSet(descriptorSetCi);
                    currFrame.boneTransformOffsets[offset.second] = descriptorSet;
                    createdOffsets.insert(offset.second);
                }
            } else if (neededBoneOffsetDescriptorSets.size() > 0) {
                for (auto const & offset : neededBoneOffsetDescriptorSets) {
                    ResourceCreationContext::DescriptorSetCreateInfo descriptorSetCi;
                    ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor bufferDescriptor;
                    bufferDescriptor.buffer = currFrame.boneTransforms;
                    bufferDescriptor.offset = offset;
                    bufferDescriptor.range = currFrame.boneTransformsSize - offset;
                    ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
                        {DescriptorType::UNIFORM_BUFFER, 0, bufferDescriptor},
                    };
                    descriptorSetCi.descriptors = descriptors;
                    descriptorSetCi.descriptorCount = 1;
                    descriptorSetCi.layout = skeletalMeshBoneLayout;
                    auto descriptorSet = ctx.CreateDescriptorSet(descriptorSetCi);
                    currFrame.boneTransformOffsets[offset] = descriptorSet;
                }
            }
            uniformCreationDone.Signal();
        });
    } else {
        uniformCreationDone.Signal();
    }
    {
        OPTICK_EVENT("UploadUniformData");
        uniformCreationDone.Wait();
        if (drawCommands.size() > 0) {
            memcpy(
                currFrame.meshIndirectMapped, drawCommands.data(), drawCommands.size() * sizeof(DrawIndirectCommand));
        }
        if (drawIndexedCommands.size() > 0) {
            memcpy(currFrame.meshIndexedIndirectMapped,
                   drawIndexedCommands.data(),
                   drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand));
        }
        size_t currentNumberOfBones = 0;
        for (size_t i = 0; i < boneSplits.size(); ++i) {
            memcpy(((uint8_t *)currFrame.boneTransformsMapped) + boneOffsets[i],
                   ((uint8_t *)&bones[currentNumberOfBones]),
                   boneSplits[i] * sizeof(glm::mat4));
            currentNumberOfBones += boneSplits[i];
        }
        /*
        if (bones.size() > 0) {
            memcpy(currFrame.boneTransformsMapped, bones.data(), bones.size() * sizeof(glm::mat4));
        }
        */
        // TODO: If this is moved up above the other memcpys the data in meshUniformsMapped somehow gets corrupted
        // and I don't understand why.
        memcpy(currFrame.meshUniformsMapped, localToWorlds.data(), localToWorlds.size() * sizeof(glm::mat4));
    }

    return batches;
}

void RenderSystem::RenderSprites(CameraInstance const & cam)
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
    currFrame.mainCommandBuffer->CmdBindVertexBuffer(quadVbo, 0, 0, sizeof(VertexWithColorAndUv));

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(passthroughTransformPipelineLayout, 0, {cam.descriptorSet});

    for (auto const & sprite : sprites) {
        if (!sprite.isActive) {
            continue;
        }
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(
            passthroughTransformPipelineLayout, 1, {sprite.descriptorSet});
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0, 0);
    }
}
