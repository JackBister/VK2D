#pragma once
#include "Core/Rendering/Backend/Abstract/RenderResources.h"

class Renderer;
class ResourceCreationContext;

class RenderPrimitiveFactory
{
public:
    RenderPrimitiveFactory(Renderer *);

    void CreatePrimitives();

private:
    RenderPassHandle * CreateMainRenderpass(ResourceCreationContext &);
    RenderPassHandle * CreatePostprocessRenderpass(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePassthroughTransformPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreatePassthroughTransformVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreateUiPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreateUiVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePostprocessPipelineLayout(ResourceCreationContext &);

#if BAKE_SHADERS
    ShaderModuleHandle * CreatePassthroughTransformVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreatePassthroughTransformFragShader(ResourceCreationContext &);
    void CreatePassthroughTransformGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass,
                                                    PipelineLayoutHandle * layout,
                                                    VertexInputStateHandle * vertexInputState,
                                                    ShaderModuleHandle * vert, ShaderModuleHandle * frag);

    ShaderModuleHandle * CreateUiVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreateUiFragShader(ResourceCreationContext &);
    void CreateUiGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass,
                                  PipelineLayoutHandle * layout, VertexInputStateHandle * vertexInputState,
                                  ShaderModuleHandle * vert, ShaderModuleHandle * frag);

    ShaderModuleHandle * CreatePassthroughNoTransformVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreatePassthroughNoTransformFragShader(ResourceCreationContext &);
    void CreatePostprocessGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass,
                                           PipelineLayoutHandle * layout, ShaderModuleHandle * vert,
                                           ShaderModuleHandle * frag);
#endif

    void CreateQuadEbo(ResourceCreationContext &);
    void CreateQuadVbo(ResourceCreationContext &);

    void CreateDefaultSampler(ResourceCreationContext &);
    void CreatePostprocessSampler(ResourceCreationContext &);

    void CreateFontImage(ResourceCreationContext &);

    Renderer * renderer;
};
