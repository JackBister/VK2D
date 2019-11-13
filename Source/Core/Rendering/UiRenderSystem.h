#pragma once

#include "Core/Rendering/Backend/Renderer.h"

#include <cstdint>
#include <vector>

class CommandBuffer;
class ResourceManager;

class UiRenderSystem
{
public:
    UiRenderSystem(Renderer * renderer, ResourceManager *);

    void StartFrame();
    void PreRenderUi(uint32_t frameIndex, CommandBuffer *);
    void RenderUi(uint32_t frameIndex, CommandBuffer *);

private:
    struct FrameInfo {
        BufferHandle * indexBuffer = nullptr;
        size_t indexBufferSize = 0;

        BufferHandle * vertexBuffer = nullptr;
        size_t vertexBufferSize = 0;
    };

    // FrameInfo related properties
    std::vector<FrameInfo> frameData;

    // Rendering resources
    DescriptorSet * descriptorSet;
    DescriptorSetLayoutHandle * layout;
    ImageHandle * fontAtlas;
    ImageViewHandle * fontAtlasView;
    PipelineHandle * gfxPipeline;
    PipelineLayoutHandle * pipelineLayout;
    SamplerHandle * fontSampler;

    // Other systems
    Renderer * renderer;
    ResourceManager * resourceManager;
};