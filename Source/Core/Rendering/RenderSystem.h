﻿#pragma once

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

  private:
    struct FrameInfo {
        FramebufferHandle * framebuffer;

        RenderPassHandle * mainRenderPass;
        uint32_t currentSubpass;

        CommandBuffer * mainCommandBuffer;
        CommandBuffer * preRenderPassCommandBuffer;

        FenceHandle * canStartFrame;
        SemaphoreHandle * framebufferReady;
        SemaphoreHandle * preRenderPassFinished;
        SemaphoreHandle * mainRenderPassFinished;

        CommandBufferAllocator * commandBufferAllocator;
    };

	void AcquireNextFrame();
    void PreRenderFrame(SubmittedFrame const & frame);
    void MainRenderFrame(SubmittedFrame const & frame);

    void PreRenderSprites(SubmittedCamera const & camera,
                          std::vector<SubmittedSprite> const & sprites);
    void RenderSprites(SubmittedCamera const & camera,
                       std::vector<SubmittedSprite> const & sprites);

    // FrameInfo related properties
    uint32_t currFrameInfoIdx = 0;
    uint32_t prevFrameInfoIdx = 0;
    std::vector<FrameInfo> frameInfo;

    // Rendering resources
    PipelineHandle * passthroughTransformPipeline;
    BufferHandle * quadEbo;
    BufferHandle * quadVbo;

    // Other systems
    Renderer * renderer;
    ResourceManager * resourceManager;
    UiRenderSystem uiRenderSystem;
};