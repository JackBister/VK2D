#pragma once

#include <glm/glm.hpp>

#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/SubmittedFrame.h"
#include "Core/Rendering/UiRenderSystem.h"
#include "Core/ResourceManager.h"

class RenderSystem
{
public:
    RenderSystem(Renderer * renderer, ResourceManager * resourceManager);

    void StartFrame();
    void RenderFrame(SubmittedFrame const & frame);

    glm::ivec2 GetResolution();

    void DebugOverrideBackbuffer(ImageViewHandle * image);

private:
    struct FrameInfo {
        FramebufferHandle * framebuffer;

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

    void AcquireNextFrame();
    void PreRenderFrame(SubmittedFrame const & frame);
    void MainRenderFrame(SubmittedFrame const & frame);
    void PostProcessFrame();

    void PreRenderCameras(std::vector<SubmittedCamera> const & cameras);

    void PreRenderSprites(std::vector<SubmittedSprite> const & sprites);
    void RenderSprites(SubmittedCamera const & camera, std::vector<SubmittedSprite> const & sprites);

    // FrameInfo related properties
    uint32_t currFrameInfoIdx = 0;
    uint32_t prevFrameInfoIdx = 0;
    std::vector<FrameInfo> frameInfo;

    // Rendering resources
    RenderPassHandle * mainRenderpass;
    RenderPassHandle * postprocessRenderpass;

    PipelineLayoutHandle * passthroughTransformPipelineLayout;
    PipelineHandle * passthroughTransformPipeline;

    SamplerHandle * postprocessSampler;
    DescriptorSetLayoutHandle * postprocessDescriptorSetLayout;
    PipelineLayoutHandle * postprocessLayout;
    PipelineHandle * postprocessPipeline;

    BufferHandle * quadEbo;
    BufferHandle * quadVbo;

    // Other systems
    Renderer * renderer;
    ResourceManager * resourceManager;
    UiRenderSystem uiRenderSystem;

    // Options
    DescriptorSet * backbufferOverride = nullptr;
};
