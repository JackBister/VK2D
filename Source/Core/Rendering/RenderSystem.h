﻿#pragma once

#include <array>
#include <set>
#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>

#include "Core/Rendering/CameraInstance.h"
#include "Core/Rendering/LightInstance.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/SkeletalMeshInstance.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Rendering/SubmeshInstance.h"
#include "Core/Rendering/UiRenderSystem.h"
#include "Jobs/JobEngine.h"
#include "RenderingBackend/Abstract/RendererConfig.h"

struct FrameContext;
class Image;
class ParticleSystem;
class Renderer;
class ShaderProgram;

struct BufferHandle;
class CommandBuffer;
class CommandBufferAllocator;
struct DescriptorSet;
struct FenceHandle;
struct FramebufferHandle;
struct ImageHandle;
struct ImageViewHandle;
struct RenderPassHandle;
class RendererProperties;
class ResourceCreationContext;
struct SamplerHandle;
struct SemaphoreHandle;

int constexpr MAX_SSAO_SAMPLES = 64;

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
    size_t boneTransformsOffset = 0; // Only used by skeletal mesh
    size_t vertexSize;
    ShaderProgram * shaderProgram;
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

struct GpuSsaoParameters {
    std::array<glm::vec4, MAX_SSAO_SAMPLES> samples;
    glm::vec2 screenResolution;
    float _padding[2];
    glm::mat4 projection;
    glm::mat4 view;
};

struct ScheduledDestroyer {
    int remainingFrames;
    std::function<void(ResourceCreationContext &)> fun;
};

class RenderSystem
{
public:
    static RenderSystem * GetInstance();

    RenderSystem(Renderer * renderer, ParticleSystem * particleSystem);

    void Init();

    void StartFrame(FrameContext & context, PreRenderCommands const &);
    void PreRenderFrame(FrameContext & context, PreRenderCommands const &);
    void RenderFrame(FrameContext & context);

    void CreateResources(std::function<void(ResourceCreationContext &)> && fun);
    void DestroyResources(std::function<void(ResourceCreationContext &)> && fun);

    glm::uvec2 GetResolution();

    CameraInstanceId CreateCamera(bool isActive = true);
    void DestroyCamera(CameraInstanceId camera);

    LightInstanceId CreatePointLightInstance(bool isActive, glm::vec3 color);
    void DestroyLightInstance(LightInstanceId id);

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

        ImageHandle * hdrColorBufferImage;
        ImageViewHandle * hdrColorBufferImageView;
        ImageHandle * normalsGBufferImage;
        ImageViewHandle * normalsGBufferImageView;
        FramebufferHandle * framebuffer;

        FramebufferHandle * ssaoFramebuffer;

        ImageViewHandle * backbuffer;
        FramebufferHandle * postprocessFramebuffer;
        DescriptorSet * tonemapDescriptorSet;

        CommandBuffer * preRenderPassCommandBuffer;
        CommandBuffer * mainCommandBuffer;
        CommandBuffer * postProcessCommandBuffer;

        FenceHandle * canStartFrame;
        SemaphoreHandle * framebufferReady;
        SemaphoreHandle * preRenderPassFinished;
        SemaphoreHandle * mainRenderPassFinished;
        SemaphoreHandle * postprocessFinished;

        CommandBufferAllocator * commandBufferAllocator;

        std::vector<JobId> preRenderJobs;

        std::vector<MeshBatch> meshBatches;

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

        size_t boneTransformsSize = 0;
        // Maps a desired offset in the boneTransforms buffer to a descriptor set with that offset
        std::unordered_map<size_t, DescriptorSet *> boneTransformOffsets;
        BufferHandle * boneTransforms = nullptr;
        glm::mat4 * boneTransformsMapped = nullptr;

        size_t lightsSize;
        BufferHandle * lights = nullptr;
        LightGpuData * lightsMapped = nullptr;
        DescriptorSet * lightsDescriptorSet = nullptr;

        size_t debugLinesSize = 0;
        BufferHandle * debugLines;
        glm::vec3 * debugLinesMapped = nullptr;

        size_t debugPointsSize = 0;
        BufferHandle * debugPoints;
        glm::vec3 * debugPointsMapped = nullptr;

        ImageHandle * ssaoOutputImage;
        ImageViewHandle * ssaoOutputImageView;
        DescriptorSet * ssaoDescriptorSet;
        ImageHandle * ssaoBlurImage;
        ImageViewHandle * ssaoBlurImageView;
        DescriptorSet * ssaoBlurDescriptorSet;
        BufferHandle * ssaoParameterBuffer;
        GpuSsaoParameters * ssaoParameterBufferMapped;
    };

    static RenderSystem * instance;

    void InitFramebuffers(ResourceCreationContext &);
    void InitSwapchainResources();

    uint32_t AcquireNextFrame(FrameContext & context);
    void MainRenderFrame(FrameContext & context);
    void PostProcessFrame(FrameContext & context);
    void SubmitSwap(FrameContext & context);

    void Prepass(FrameContext & context, std::vector<MeshBatch> const & batches);

    void PreRenderCameras(FrameContext const & context, std::vector<UpdateCamera> const & cameras);

    void PreRenderLights(std::vector<UpdateLight> const & lights);
    void UpdateLights(FrameContext const & context);

    void PreRenderSkeletalMeshes(std::vector<UpdateSkeletalMeshInstance> const & meshes);

    void PreRenderSprites(FrameContext const & context, std::vector<UpdateSpriteInstance> const & sprites);
    void RenderSprites(FrameContext & context, CameraInstance const & camera);

    void PreRenderMeshes(FrameContext const & context, std::vector<UpdateStaticMeshInstance> const & meshes);
    void RenderMeshes(FrameContext & context, CameraInstance const & camera, std::vector<MeshBatch> const & batches);
    void RenderTransparentMeshes(FrameContext & context, CameraInstance const & camera,
                                 std::vector<MeshBatch> const & batches);

    void PreRenderSSAO(FrameContext const & context);

    void RenderDebugDraws(FrameContext & context, CameraInstance const & camera);

    void CreateBatches(FrameContext & context);

    std::vector<FrameInfo> frameInfo;

    std::vector<ScheduledDestroyer> scheduledDestroyers;

    // Rendering resources
    RenderPassHandle * prepass = nullptr;
    RenderPassHandle * mainRenderpass;
    RenderPassHandle * postprocessRenderpass;

    PipelineLayoutHandle * prepassPipelineLayout = nullptr;
    ShaderProgram * prepassProgram = nullptr;

    PipelineLayoutHandle * skeletalPrepassPipelineLayout = nullptr;
    ShaderProgram * skeletalPrepassProgram = nullptr;

    PipelineLayoutHandle * passthroughTransformPipelineLayout;
    ShaderProgram * passthroughTransformProgram;

    DescriptorSetLayoutHandle * lightsLayout;

    DescriptorSetLayoutHandle * meshModelLayout;
    PipelineLayoutHandle * meshPipelineLayout;
    ShaderProgram * meshProgram;
    ShaderProgram * transparentMeshProgram;

    DescriptorSetLayoutHandle * skeletalMeshBoneLayout;
    PipelineLayoutHandle * skeletalMeshPipelineLayout;
    ShaderProgram * skeletalMeshProgram;

    ShaderProgram * uiProgram;

    DescriptorSetLayoutHandle * ambientOcclusionDescriptorSetLayout;
    PipelineLayoutHandle * ambientOcclusionLayout;
    ShaderProgram * ambientOcclusionProgram;

    DescriptorSetLayoutHandle * ambientOcclusionBlurDescriptorSetLayout;
    PipelineLayoutHandle * ambientOcclusionBlurLayout;
    ShaderProgram * ambientOcclusionBlurProgram;

    DescriptorSetLayoutHandle * tonemapDescriptorSetLayout;
    PipelineLayoutHandle * tonemapLayout;
    ShaderProgram * tonemapProgram;

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

    // lights
    std::vector<LightInstance> lights;
    LightInstance * GetLight(LightInstanceId);

    // sprites
    std::vector<SpriteInstance> sprites;
    SpriteInstance * GetSpriteInstance(SpriteInstanceId);

    // skeletal meshes
    std::vector<SkeletalMeshInstance> skeletalMeshes;
    SkeletalMeshInstance * GetSkeletalMeshInstance(SkeletalMeshInstanceId id);
    void ReadNodeHierarchy(float animationTime, SkeletalMeshAnimation const * animation,
                           SkeletalMeshInstance * skeleton, SkeletalBoneInstance * bone,
                           const glm::mat4x4 & parentTransform);
    void TransformVertices(SkeletalMeshInstance * instance);
    void UpdateAnimations();
    void UpdateAnimation(SkeletalMeshInstance * instance, float dt);

    // static meshes
    std::set<SubmeshInstance, SubmeshInstanceComparer> sortedSubmeshInstances;
    std::vector<StaticMeshInstance> staticMeshes;
    StaticMeshInstance * GetStaticMeshInstance(StaticMeshInstanceId);

    // SSAO
    ImageHandle * ssaoNoiseImage;
    ImageViewHandle * ssaoNoiseImageView;
    RenderPassHandle * ssaoPass;

    // Other systems
    JobEngine * jobEngine;
    Renderer * renderer;
    RendererProperties const & rendererProperties;
    UiRenderSystem uiRenderSystem;
    ParticleSystem * particleSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
    std::optional<RendererConfig> queuedConfigUpdate;
};
