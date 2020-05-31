#pragma once
#include "Core/Rendering/Backend/Abstract/RenderResources.h"

class Renderer;
class ResourceCreationContext;

class RenderPrimitiveFactory
{
public:
    RenderPrimitiveFactory(Renderer *);

    void CreatePrimitives();
    // This is stupid
    void LateCreatePrimitives();

private:
    void CreateDebugDrawPointVertexInputState(ResourceCreationContext &);
    void CreateDebugDrawPipelineLayout(ResourceCreationContext &);

    RenderPassHandle * CreateMainRenderpass(ResourceCreationContext &);
    RenderPassHandle * CreatePostprocessRenderpass(ResourceCreationContext &);

    void CreatePrepassPipelineLayout(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePassthroughTransformPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreatePassthroughTransformVertexInputState(ResourceCreationContext &);

    void CreateMeshPipelineLayout(ResourceCreationContext &);
    void CreateMeshVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreateUiPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreateUiVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePostprocessPipelineLayout(ResourceCreationContext &);

    void CreateDefaultSampler(ResourceCreationContext &);
    void CreatePostprocessSampler(ResourceCreationContext &);

    void CreateFontImage(ResourceCreationContext &);

    void CreateDefaultMaterial(ResourceCreationContext &);

    void CreateQuadEbo(ResourceCreationContext &);
    void CreateQuadVbo(ResourceCreationContext &);
    void CreateQuadMesh();

    void CreateBoxVbo(ResourceCreationContext &);
    void CreateBoxMesh();

    Renderer * renderer;
};
