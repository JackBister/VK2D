﻿#pragma once

#include <set>
#include <vector>

#include <glm/glm.hpp>

#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/CameraInstance.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/SkeletalMeshInstance.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Rendering/SubmeshInstance.h"
#include "Core/Rendering/UiRenderSystem.h"

class Image;
class ShaderProgram;

struct DrawIndirectCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct DrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};

struct MeshBatch {
    BufferHandle * indexBuffer = nullptr;
    BufferHandle * vertexBuffer = nullptr;
    Material * material = nullptr;
    size_t drawCommandsOffset;
    size_t drawCommandsCount;
    size_t drawIndexedCommandsOffset;
    size_t drawIndexedCommandsCount;
    std::vector<DrawIndirectCommand> drawCommands;
    std::vector<DrawIndexedIndirectCommand> drawIndexedCommands;
};

struct ScheduledDestroyer {
    int remainingFrames;
    std::function<void(ResourceCreationContext &)> fun;
};

class RenderSystem
{
public:
    static RenderSystem * GetInstance();

    RenderSystem(Renderer * renderer);

    void Init();

    void StartFrame();
    void PreRenderFrame(PreRenderCommands);
    void RenderFrame();

    void CreateResources(std::function<void(ResourceCreationContext &)> fun);
    void DestroyResources(std::function<void(ResourceCreationContext &)> fun);

    glm::ivec2 GetResolution();

    CameraInstanceId CreateCamera(bool isActive = true);
    void DestroyCamera(CameraInstanceId camera);

    SkeletalMeshInstanceId CreateSkeletalMeshInstance(SkeletalMesh * mesh, bool isActive = true);
    void DestroySkeletalMeshInstance(SkeletalMeshInstanceId id);

    SpriteInstanceId CreateSpriteInstance(Image * image, bool isActive = true);
    void DestroySpriteInstance(SpriteInstanceId spriteInstance);

    StaticMeshInstanceId CreateStaticMeshInstance(StaticMesh * mesh, bool isActive = true);
    void DestroyStaticMeshInstance(StaticMeshInstanceId staticMesh);

    void DebugOverrideBackbuffer(ImageViewHandle * image);

private:
    struct FrameInfo {
        ImageHandle * prepassDepthImage;
        ImageViewHandle * prepassDepthImageView;
        FramebufferHandle * prepassFramebuffer;

        ImageHandle * normalsGBufferImage;
        ImageViewHandle * normalsGBufferImageView;
        ImageViewHandle * backbuffer;
        FramebufferHandle * framebuffer;

        FramebufferHandle * postprocessFramebuffer;

        CommandBuffer * preRenderPassCommandBuffer;
        CommandBuffer * mainCommandBuffer;
        CommandBuffer * postProcessCommandBuffer;

        FenceHandle * canStartFrame;
        SemaphoreHandle * framebufferReady;
        SemaphoreHandle * preRenderPassFinished;
        SemaphoreHandle * mainRenderPassFinished;
        SemaphoreHandle * postprocessFinished;

        CommandBufferAllocator * commandBufferAllocator;

        // Contains per-mesh uniform info (such as localToWorld matrix)
        size_t meshUniformsSize = 0;
        BufferHandle * meshUniforms = nullptr;
        glm::mat4 * meshUniformsMapped = nullptr;
        DescriptorSet * meshUniformsDescriptorSet = nullptr;
        size_t meshIndirectSize = 0;
        BufferHandle * meshIndirect = nullptr;
        DrawIndirectCommand * meshIndirectMapped = nullptr;
        size_t meshIndexedIndirectSize = 0;
        BufferHandle * meshIndexedIndirect = nullptr;
        DrawIndexedIndirectCommand * meshIndexedIndirectMapped = nullptr;

        size_t debugLinesSize = 0;
        BufferHandle * debugLines;
        glm::vec3 * debugLinesMapped = nullptr;

        size_t debugPointsSize = 0;
        BufferHandle * debugPoints;
        glm::vec3 * debugPointsMapped = nullptr;
    };

    static RenderSystem * instance;

    void InitFramebuffers(ResourceCreationContext &);
    void InitSwapchainResources();

    uint32_t AcquireNextFrame();
    void MainRenderFrame();
    void PostProcessFrame();
    void SubmitSwap();

    void Prepass(std::vector<MeshBatch> const & batches);

    void PreRenderCameras(std::vector<UpdateCamera> const & cameras);

    void PreRenderSkeletalMeshes(std::vector<UpdateSkeletalMeshInstance> const & meshes);

    void PreRenderSprites(std::vector<UpdateSpriteInstance> const & sprites);
    void RenderSprites(CameraInstance const & camera);

    void PreRenderMeshes(std::vector<UpdateStaticMeshInstance> const & meshes);
    void RenderMeshes(CameraInstance const & camera, std::vector<MeshBatch> const & batches);
    void RenderTransparentMeshes(CameraInstance const & camera, std::vector<MeshBatch> const & batches);

    void RenderDebugDraws(CameraInstance const & camera);

    std::vector<MeshBatch> CreateBatches();

    // FrameInfo related properties
    uint32_t currFrameInfoIdx = 0;
    uint32_t prevFrameInfoIdx = 0;
    std::vector<FrameInfo> frameInfo;

    std::vector<ScheduledDestroyer> scheduledDestroyers;

    // Rendering resources
    RenderPassHandle * prepass = nullptr;
    RenderPassHandle * mainRenderpass;
    RenderPassHandle * postprocessRenderpass;

    PipelineLayoutHandle * prepassPipelineLayout = nullptr;
    ShaderProgram * prepassProgram = nullptr;

    PipelineLayoutHandle * passthroughTransformPipelineLayout;
    ShaderProgram * passthroughTransformProgram;

    DescriptorSetLayoutHandle * meshModelLayout;
    PipelineLayoutHandle * meshPipelineLayout;
    ShaderProgram * meshProgram;
    ShaderProgram * transparentMeshProgram;

    ShaderProgram * uiProgram;

    SamplerHandle * postprocessSampler;
    DescriptorSetLayoutHandle * postprocessDescriptorSetLayout;
    PipelineLayoutHandle * postprocessLayout;
    ShaderProgram * postprocessProgram;

    PipelineLayoutHandle * debugDrawLayout;
    ShaderProgram * debugDrawLinesProgram;
    ShaderProgram * debugDrawPointsProgram;

    BufferHandle * quadEbo;
    BufferHandle * quadVbo;

    // cameras
    std::vector<CameraInstance> cameras;
    CameraInstance * GetCamera(CameraInstanceId);

    // sprites
    std::vector<SpriteInstance> sprites;
    SpriteInstance * GetSpriteInstance(SpriteInstanceId);

    // skeletal meshes
    std::vector<SkeletalMeshInstance> skeletalMeshes;
    SkeletalMeshInstance * GetSkeletalMeshInstance(SkeletalMeshInstanceId id);
    void UpdateAnimations();

    // static meshes
    std::set<SubmeshInstance, SubmeshInstanceComparer> sortedSubmeshInstances;
    std::vector<StaticMeshInstance> staticMeshes;
    StaticMeshInstance * GetStaticMeshInstance(StaticMeshInstanceId);

    // Other systems
    Renderer * renderer;
    UiRenderSystem uiRenderSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
    std::optional<RendererConfig> queuedConfigUpdate;
};
