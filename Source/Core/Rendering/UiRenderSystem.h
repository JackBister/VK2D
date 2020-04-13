#pragma once

#include "Core/Rendering/Backend/Renderer.h"

#include <cstdint>
#include <vector>

class CommandBuffer;
class ShaderProgram;

class UiRenderSystem
{
public:
    UiRenderSystem(Renderer * renderer);

    void Init();

    void StartFrame();
    void PreRenderUi(uint32_t frameIndex, CommandBuffer *);
    void RenderUi(uint32_t frameIndex, CommandBuffer *);

private:
    struct FrameInfo {
        BufferHandle * indexBuffer = nullptr;
        uint8_t * indexBufferMapped = nullptr;
        size_t indexBufferSize = 0;

        BufferHandle * vertexBuffer = nullptr;
        uint8_t * vertexBufferMapped = nullptr;
        size_t vertexBufferSize = 0;
    };

    void RecreateBuffers(uint32_t frameIndex, size_t totalVertexSize, size_t totalIndexSize);

    // FrameInfo related properties
    std::vector<FrameInfo> frameData;

    // Rendering resources
    BufferHandle * resolutionUniformBuffer;
    DescriptorSet * descriptorSet;
    DescriptorSetLayoutHandle * layout;
    ShaderProgram * uiProgram;
    PipelineLayoutHandle * pipelineLayout;

    // Other systems
    Renderer * renderer;
};