#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/CameraInstance.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Rendering/SubmittedFrame.h"
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
    void RenderFrame(SubmittedFrame const & frame);

    void CreateResources(std::function<void(ResourceCreationContext &)> fun);
    void DestroyResources(std::function<void(ResourceCreationContext &)> fun);

    glm::ivec2 GetResolution();

    CameraInstanceId CreateCamera();
    void DestroyCamera(CameraInstanceId camera);

    SpriteInstanceId CreateSpriteInstance(Image * image);
    void DestroySpriteInstance(SpriteInstanceId spriteInstance);

    StaticMeshInstanceId CreateStaticMeshInstance(StaticMesh * mesh);
    void DestroyStaticMeshInstance(StaticMeshInstanceId staticMesh);

    void DebugOverrideBackbuffer(ImageViewHandle * image);

private:
    struct FrameInfo {
        ImageHandle * prepassDepthImage;
        ImageViewHandle * prepassDepthImageView;
        FramebufferHandle * prepassFramebuffer;

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
    };

    static RenderSystem * instance;

    void InitFramebuffers(ResourceCreationContext &);
    void InitSwapchainResources();

    uint32_t AcquireNextFrame();
    void MainRenderFrame(SubmittedFrame const & frame);
    void PostProcessFrame();
    void SubmitSwap();

    void Prepass(SubmittedFrame const & frame, std::vector<MeshBatch> const & batches);

    void PreRenderCameras(std::vector<UpdateCamera> const & cameras);

    void PreRenderSprites(std::vector<UpdateSpriteInstance> const & sprites);
    void RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites);

    void PreRenderMeshes(std::vector<UpdateStaticMeshInstance> const & meshes);
    void RenderMeshes(SubmittedCamera const & camera, std::vector<MeshBatch> const & batches);
    void RenderTransparentMeshes(SubmittedCamera const & camera, std::vector<MeshBatch> const & batches);

    std::vector<MeshBatch> CreateBatches(std::vector<SubmittedMesh> const & meshes);

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

    BufferHandle * quadEbo;
    BufferHandle * quadVbo;

    // cameras
    std::vector<CameraInstance> cameras;
    CameraInstance * GetCamera(CameraInstanceId);

    // sprites
    std::vector<SpriteInstance> sprites;
    SpriteInstance * GetSpriteInstance(SpriteInstanceId);

    // static meshes
    std::vector<StaticMeshInstance> staticMeshes;
    StaticMeshInstance * GetStaticMeshInstance(StaticMeshInstanceId);

    // Other systems
    Renderer * renderer;
    UiRenderSystem uiRenderSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
    std::optional<RendererConfig> queuedConfigUpdate;
};
