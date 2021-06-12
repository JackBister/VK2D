#pragma once

class Renderer;
class ResourceCreationContext;

struct RenderPassHandle;
struct PipelineLayoutHandle;
struct VertexInputStateHandle;

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
    RenderPassHandle * CreateSSAORenderpass(ResourceCreationContext &);
    RenderPassHandle * CreatePostprocessRenderpass(ResourceCreationContext &);

    void CreatePrepassPipelineLayout(ResourceCreationContext &);
    void CreateSkeletalPrepassPipelineLayout(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePassthroughTransformPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreatePassthroughTransformVertexInputState(ResourceCreationContext &);

    void CreateMeshPipelineLayout(ResourceCreationContext &);
    void CreateMeshVertexInputState(ResourceCreationContext &);

    void CreateSkeletalMeshPipelineLayout(ResourceCreationContext &);
    void CreateSkeletalMeshVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreateAmbientOcclusionPipelineLayout(ResourceCreationContext &);
    PipelineLayoutHandle * CreateAmbientOcclusionBlurPipelineLayout(ResourceCreationContext &);
    PipelineLayoutHandle * CreateTonemapPipelineLayout(ResourceCreationContext &);

    PipelineLayoutHandle * CreateUiPipelineLayout(ResourceCreationContext &);
    VertexInputStateHandle * CreateUiVertexInputState(ResourceCreationContext &);

    PipelineLayoutHandle * CreatePostprocessPipelineLayout(ResourceCreationContext &);

    void CreateDefaultSampler(ResourceCreationContext &);
    void CreatePostprocessSampler(ResourceCreationContext &);

    void CreateFontImage(ResourceCreationContext &);

    void CreateDefaultNormalMap(ResourceCreationContext &);
    void CreateDefaultRoughnessMap(ResourceCreationContext &);
    void CreateDefaultMetallicMap(ResourceCreationContext &);
    void CreateDefaultMaterial(ResourceCreationContext &);

    void CreateQuadEbo(ResourceCreationContext &);
    void CreateQuadVbo(ResourceCreationContext &);
    void CreateQuadMesh();

    void CreateBoxVbo(ResourceCreationContext &);
    void CreateBoxMesh();

    Renderer * renderer;
};
