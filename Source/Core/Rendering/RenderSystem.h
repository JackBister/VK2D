#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/CameraHandle.h"
#include "Core/Rendering/CameraResources.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Core/Rendering/SpriteInstanceResources.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Rendering/StaticMeshInstanceResources.h"
#include "Core/Rendering/SubmittedFrame.h"
#include "Core/Rendering/UiRenderSystem.h"

class Image;
class ShaderProgram;

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

    CameraHandle CreateCamera();
    void DestroyCamera(CameraHandle camera);

    SpriteInstance CreateSpriteInstance(Image * image);
    void DestroySpriteInstance(SpriteInstance spriteInstance);

    StaticMeshInstance CreateStaticMeshInstance();
    void DestroyStaticMeshInstance(StaticMeshInstance staticMesh);

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
    };

    static RenderSystem * instance;

    void InitFramebuffers(ResourceCreationContext &);
    void InitSwapchainResources();

    uint32_t AcquireNextFrame();
    void MainRenderFrame(SubmittedFrame const & frame);
    void PostProcessFrame();
    void SubmitSwap();

    void Prepass(SubmittedFrame const & frame);

    void PreRenderCameras(std::vector<UpdateCamera> const & cameras);

    void PreRenderSprites(std::vector<UpdateSpriteInstance> const & sprites);
    void RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites);

    void PreRenderMeshes(std::vector<UpdateStaticMeshInstance> const & meshes);
    void RenderMeshes(SubmittedCamera const & camera, std::vector<SubmittedMesh> const & meshes);
    void RenderTransparentMeshes(SubmittedCamera const & camera, std::vector<SubmittedMesh> const & meshes);

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
    std::vector<CameraResources> cameras;
    CameraResources * GetCamera(CameraHandle);

    // sprites
    std::vector<SpriteInstanceResources> sprites;
    SpriteInstanceResources * GetSpriteInstance(SpriteInstance);

    // static meshes
    std::vector<StaticMeshInstanceResources> staticMeshes;
    StaticMeshInstanceResources * GetStaticMeshInstance(StaticMeshInstance);

    // Other systems
    Renderer * renderer;
    UiRenderSystem uiRenderSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
    std::optional<RendererConfig> queuedConfigUpdate;
};
