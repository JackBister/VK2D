#pragma once

#include <cstdint>
#include <vector>

#include "Core/Rendering/Backend/Renderer.h"

class CommandBuffer;
struct FrameContext;
class ShaderProgram;

class UiRenderSystem
{
public:
    static UiRenderSystem * GetInstance();

    UiRenderSystem(Renderer * renderer);

    void Init();

    // Imgui calls must only be made between StartFrame and EndFrame on a given frame.
    void StartFrame();
    void EndFrame(FrameContext & context);
    void PreRenderUi(FrameContext const & context, CommandBuffer *);
    void RenderUi(FrameContext & context, CommandBuffer *);

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

    static UiRenderSystem * instance;

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