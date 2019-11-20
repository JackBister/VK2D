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

    ShaderModuleHandle * CreatePassthroughTransformVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreatePassthroughTransformFragShader(ResourceCreationContext &);
    void CreatePassthroughTransformGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass,
                                                    ShaderModuleHandle * vert, ShaderModuleHandle * frag);

    ShaderModuleHandle * CreateUiVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreateUiFragShader(ResourceCreationContext &);
    void CreateUiGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass, ShaderModuleHandle * vert,
                                  ShaderModuleHandle * frag);

    ShaderModuleHandle * CreatePassthroughNoTransformVertShader(ResourceCreationContext &);
    ShaderModuleHandle * CreatePassthroughNoTransformFragShader(ResourceCreationContext &);
    void CreatePostprocessGraphicsPipeline(ResourceCreationContext &, RenderPassHandle * renderpass,
                                           ShaderModuleHandle * vert, ShaderModuleHandle * frag);

    void CreateQuadEbo(ResourceCreationContext &);
    void CreateQuadVbo(ResourceCreationContext &);

    void CreateDefaultSampler(ResourceCreationContext &);

    void CreateFontImage(ResourceCreationContext &);

    Renderer * renderer;
};
