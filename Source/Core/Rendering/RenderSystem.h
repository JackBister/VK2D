#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/SubmittedFrame.h"
#include "Core/Rendering/UiRenderSystem.h"

class ShaderProgram;

struct ScheduledDestroyer {
    int remainingFrames;
    std::function<void(ResourceCreationContext &)> fun;
};

class RenderSystem
{
public:
    RenderSystem(Renderer * renderer);

    void Init();

    void StartFrame();
    void RenderFrame(SubmittedFrame const & frame);

    void CreateResources(std::function<void(ResourceCreationContext &)> fun);
    void DestroyResources(std::function<void(ResourceCreationContext &)> fun);

    glm::ivec2 GetResolution();

    void DebugOverrideBackbuffer(ImageViewHandle * image);

private:
    struct FrameInfo {
        FramebufferHandle * framebuffer;
        ImageViewHandle * backbuffer;

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

    void InitFramebuffers(ResourceCreationContext &);
    void InitSwapchainResources();

    uint32_t AcquireNextFrame();
    void PreRenderFrame(SubmittedFrame const & frame);
    void MainRenderFrame(SubmittedFrame const & frame);
    void PostProcessFrame();

    void PreRenderCameras(std::vector<SubmittedCamera> const & cameras);

    void PreRenderMeshes(std::vector<SubmittedMesh> const & meshes);
    void RenderMeshes(SubmittedCamera const & camera, std::vector<SubmittedMesh> const & meshes);

    void PreRenderSprites(std::vector<SubmittedSprite> const & sprites);
    void RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites);

    // FrameInfo related properties
    uint32_t currFrameInfoIdx = 0;
    uint32_t prevFrameInfoIdx = 0;
    std::vector<FrameInfo> frameInfo;

    std::vector<ScheduledDestroyer> scheduledDestroyers;

    // Rendering resources
    RenderPassHandle * mainRenderpass;
    RenderPassHandle * postprocessRenderpass;

    PipelineLayoutHandle * passthroughTransformPipelineLayout;
    ShaderProgram * passthroughTransformProgram;

    PipelineLayoutHandle * meshPipelineLayout;
    ShaderProgram * meshProgram;

    ShaderProgram * uiProgram;

    SamplerHandle * postprocessSampler;
    DescriptorSetLayoutHandle * postprocessDescriptorSetLayout;
    PipelineLayoutHandle * postprocessLayout;
    ShaderProgram * postprocessProgram;

    BufferHandle * quadEbo;
    BufferHandle * quadVbo;

    // Other systems
    Renderer * renderer;
    UiRenderSystem uiRenderSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
    std::optional<RendererConfig> queuedConfigUpdate;
};
