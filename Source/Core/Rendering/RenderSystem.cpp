#include "Core/Rendering/RenderSystem.h"

#include <set>

#include <glm/gtc/type_ptr.hpp>
#include <optick/optick.h>

#include "Core/Logging/Logger.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ShaderProgram.h"
#include "Core/Resources/StaticMesh.h"
#include "Core/Semaphore.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("RenderSystem");

static constexpr CommandBuffer::ClearValue DEFAULT_CLEAR_VALUES[] = {
    {CommandBuffer::ClearValue::Type::COLOR, {0.f, 0.f, 1.f, 1.f}}};

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

    auto meshBatches = CreateBatches();

    Prepass(frame, meshBatches);

    auto res = renderer->GetResolution();
    CommandBuffer::RenderPassBeginInfo beginInfo = {
        mainRenderpass, currFrame.framebuffer, {{0, 0}, {res.x, res.y}}, 1, DEFAULT_CLEAR_VALUES};
    currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::INLINE);
    for (auto const & camera : cameras) {
        if (!camera.isActive) {
            continue;
        }
        RenderMeshes(camera, meshBatches);
        RenderSprites(camera, frame.sprites);

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

void RenderSystem::Prepass(SubmittedFrame const & frame, std::vector<MeshBatch> const & batches)
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

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 prepassProgram->GetPipeline());

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

            currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, 8 * sizeof(float));
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
                    currFrame.mainCommandBuffer->CmdDrawIndexed(
                        command.indexCount, command.instanceCount, command.firstIndex, command.vertexOffset);
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
        auto pv = camera.projection * camera.view;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            cam->uniformBuffer, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(pv));
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
    auto & currFrame = frameInfo[currFrameInfoIdx];

    if (!currFrame.meshUniformsDescriptorSet) {
        return;
    }

    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 meshProgram->GetPipeline());

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(
        meshPipelineLayout, 0, {cam.descriptorSet, currFrame.meshUniformsDescriptorSet});

    DescriptorSet * currentMaterialDescriptorSet = nullptr;
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
                meshPipelineLayout, 2, {batch.material->GetDescriptorSet()});
        }

        currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, 8 * sizeof(float));
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
                currFrame.mainCommandBuffer->CmdDrawIndexed(
                    command.indexCount, command.instanceCount, command.firstIndex, command.vertexOffset);
            }
        }
    }
}

void RenderSystem::RenderTransparentMeshes(CameraInstance const & cam, std::vector<MeshBatch> const & batches)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[currFrameInfoIdx];

    if (!currFrame.meshUniformsDescriptorSet) {
        return;
    }

    auto res = renderer->GetResolution();

    CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
    currFrame.mainCommandBuffer->CmdSetViewport(0, 1, &viewport);

    CommandBuffer::Rect2D scissor = {{0, 0}, {res.x, res.y}};
    currFrame.mainCommandBuffer->CmdSetScissor(0, 1, &scissor);

    currFrame.mainCommandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                 transparentMeshProgram->GetPipeline());

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(
        meshPipelineLayout, 0, {cam.descriptorSet, currFrame.meshUniformsDescriptorSet});

    DescriptorSet * currentMaterialDescriptorSet = nullptr;
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
                meshPipelineLayout, 2, {batch.material->GetDescriptorSet()});
        }

        currFrame.mainCommandBuffer->CmdBindVertexBuffer(batch.vertexBuffer, 0, 0, 8 * sizeof(float));
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
                currFrame.mainCommandBuffer->CmdDrawIndexed(
                    command.indexCount, command.instanceCount, command.firstIndex, command.vertexOffset);
            }
        }
    }
}

// Absolute caveman code here
struct SubmeshWithUniformId {
    Submesh const * submesh;
    uint32_t id;
};

struct SubmeshComparer {
    bool operator()(SubmeshWithUniformId const & lhs, SubmeshWithUniformId const & rhs) const
    {
        if (lhs.submesh->GetMaterial()->GetDescriptorSet() < rhs.submesh->GetMaterial()->GetDescriptorSet()) {
            return true;
        }
        if (lhs.submesh->GetMaterial()->GetDescriptorSet() > rhs.submesh->GetMaterial()->GetDescriptorSet()) {
            return false;
        }
        // Same material
        if (lhs.submesh->GetVertexBuffer().GetBuffer() < rhs.submesh->GetVertexBuffer().GetBuffer()) {
            return true;
        }
        if (lhs.submesh->GetVertexBuffer().GetBuffer() > rhs.submesh->GetVertexBuffer().GetBuffer()) {
            return false;
        }
        // Same material, same vertex buffer
        if (!lhs.submesh->GetIndexBuffer().has_value() && !rhs.submesh->GetIndexBuffer().has_value()) {
            // Same material, same vertex buffer, no index buffers
            if (lhs.submesh->GetVertexBuffer().GetOffset() < rhs.submesh->GetVertexBuffer().GetOffset()) {
                return true;
            }
            if (lhs.submesh->GetVertexBuffer().GetOffset() > rhs.submesh->GetVertexBuffer().GetOffset()) {
                return false;
            }
            if (lhs.id < rhs.id) {
                return true;
            }
            if (lhs.id > rhs.id) {
                return false;
            }
            // Same material, same vertex buffer, no index buffers, same offset, same id
            return lhs.submesh->GetNumVertices() < rhs.submesh->GetNumVertices();
        }
        if (lhs.submesh->GetIndexBuffer().has_value() && !rhs.submesh->GetIndexBuffer().has_value()) {
            return true;
        }
        if (!lhs.submesh->GetIndexBuffer().has_value() && rhs.submesh->GetIndexBuffer().has_value()) {
            return false;
        }
        // Same material, same vertex buffer, both have index buffer
        if (lhs.submesh->GetIndexBuffer().value().GetBuffer() < rhs.submesh->GetIndexBuffer().value().GetBuffer()) {
            return true;
        }
        if (lhs.submesh->GetIndexBuffer().value().GetBuffer() > rhs.submesh->GetIndexBuffer().value().GetBuffer()) {
            return false;
        }
        // Same material, same vertex buffer, same index buffer
        if (lhs.submesh->GetIndexBuffer().value().GetOffset() < rhs.submesh->GetIndexBuffer().value().GetOffset()) {
            return true;
        }
        if (lhs.submesh->GetIndexBuffer().value().GetOffset() > rhs.submesh->GetIndexBuffer().value().GetOffset()) {
            return false;
        }
        if (lhs.id < rhs.id) {
            return true;
        }
        if (lhs.id > rhs.id) {
            return false;
        }
        // Same material, same vertex buffer, same index buffer, same index offset, same id
        return lhs.submesh->GetNumIndexes() < rhs.submesh->GetNumIndexes();
    }
};

std::vector<MeshBatch> RenderSystem::CreateBatches()
{
    OPTICK_EVENT();

    std::vector<glm::mat4> localToWorlds;
    std::set<SubmeshWithUniformId, SubmeshComparer> sortedSubmeshes;
    {
        OPTICK_EVENT("SortSubmeshes");
        for (auto const & instance : this->staticMeshes) {
            if (!instance.isActive) {
                continue;
            }
            localToWorlds.push_back(instance.localToWorld);
            uint32_t id = localToWorlds.size() - 1;
            for (auto const & submesh : instance.mesh->GetSubmeshes()) {
                sortedSubmeshes.insert({&submesh, id});
            }
        }
    }

    auto & currFrame = frameInfo[currFrameInfoIdx];

    std::vector<MeshBatch> batches;
    size_t drawCommandsOffset = 0;
    std::vector<DrawIndirectCommand> drawCommands;
    size_t drawIndexedOffset = 0;
    std::vector<DrawIndexedIndirectCommand> drawIndexedCommands;
    {
        OPTICK_EVENT("GatherBatches");
        MeshBatch currentBatch;
        for (auto const & submesh : sortedSubmeshes) {
            if (submesh.submesh->GetMaterial() != currentBatch.material ||
                submesh.submesh->GetVertexBuffer().GetBuffer() != currentBatch.vertexBuffer ||
                (submesh.submesh->GetIndexBuffer().has_value() ? submesh.submesh->GetIndexBuffer()->GetBuffer()
                                                               : nullptr) != currentBatch.indexBuffer) {
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
                    (submesh.submesh->GetIndexBuffer().has_value() ? submesh.submesh->GetIndexBuffer()->GetBuffer()
                                                                   : nullptr);
                currentBatch.material = submesh.submesh->GetMaterial();
                currentBatch.vertexBuffer = submesh.submesh->GetVertexBuffer().GetBuffer();
            }
            if (submesh.submesh->GetIndexBuffer().has_value()) {
                DrawIndexedIndirectCommand command;
                command.firstIndex = submesh.submesh->GetIndexBuffer()->GetOffset();
                command.firstInstance = submesh.id;
                command.indexCount = submesh.submesh->GetNumIndexes();
                command.instanceCount = 1;
                command.vertexOffset = submesh.submesh->GetVertexBuffer().GetOffset() / (8 * sizeof(float));
                currentBatch.drawIndexedCommands.push_back(command);
                drawIndexedCommands.push_back(command);
            } else {
                DrawIndirectCommand command;
                command.firstInstance = submesh.id;
                command.firstVertex = submesh.submesh->GetVertexBuffer().GetOffset() / (8 * sizeof(float));
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
        drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand) > currFrame.meshIndexedIndirectSize) {
        renderer->CreateResources(
            [this, &uniformCreationDone, &currFrame, &drawCommands, &drawIndexedCommands, &localToWorlds](
                ResourceCreationContext & ctx) {
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
                if (drawIndexedCommands.size() * sizeof(DrawIndexedIndirectCommand) >
                    currFrame.meshIndexedIndirectSize) {
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
        // TODO: If this is moved up above the other memcpys the data in meshUniformsMapped somehow gets corrupted
        // and I don't understand why.
        memcpy(currFrame.meshUniformsMapped, localToWorlds.data(), localToWorlds.size() * sizeof(glm::mat4));
    }

    return batches;
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

void RenderSystem::RenderSprites(CameraInstance const & cam, std::vector<SubmittedSprite> const & sprites)
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

    currFrame.mainCommandBuffer->CmdBindDescriptorSets(passthroughTransformPipelineLayout, 0, {cam.descriptorSet});

    for (auto const & sprite : sprites) {
        auto spriteInstance = GetSpriteInstance(sprite.spriteInstance);
        currFrame.mainCommandBuffer->CmdBindDescriptorSets(
            passthroughTransformPipelineLayout, 1, {spriteInstance->descriptorSet});
        currFrame.mainCommandBuffer->CmdDrawIndexed(6, 1, 0, 0);
    }
}
