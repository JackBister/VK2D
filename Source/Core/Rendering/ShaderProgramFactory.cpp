#include "ShaderProgramFactory.h"

#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"

void ShaderProgramFactory::CreateResources()
{
    CreateDebugDrawShaderProgram();
    CreatePassthroughTransformShaderProgram();
    CreateMeshShaderProgram();
    CreateSkeletalMeshShaderProgram();
    CreateTransparentMeshShaderProgram();
    CreateAmbientOcclusionProgram();
    CreateAmbientOcclusionBlurProgram();
    CreateParticleRenderingProgram();
    CreateTonemapProgram();
    CreateUiShaderProgram();
    CreatePostprocessShaderProgram();
}
void ShaderProgramFactory::CreateDebugDrawShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::NEVER;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/debug_draw_lines.program",
        {"shaders/debug_draw.vert", "shaders/debug_draw.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/debug_draw.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/debug_draw.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        1,
        {
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::LINE_LIST,
        },
        depthStencil);

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/debug_draw_points.program",
        {"shaders/debug_draw.vert", "shaders/debug_draw.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/debug_draw.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/debug_draw.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        1,
        {
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::POINT_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreatePassthroughTransformShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/passthrough-transform.program",
        {"shaders/passthrough-transform.vert", "shaders/passthrough-transform.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        0,
        {
            // Blending is enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
            // disabled for normals
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateMeshShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::EQUAL;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = false;

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/mesh.program",
        {"shaders/mesh.vert", "shaders/mesh.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/mesh.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::COUNTER_CLOCKWISE,
        0,
        {
            // Blending is enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
            // disabled for normals
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateSkeletalMeshShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::EQUAL;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = false;

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/mesh_skeletal.program",
        {"shaders/mesh_skeletal.vert", "shaders/mesh.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/mesh_skeletal.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh_skeletal.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::COUNTER_CLOCKWISE,
        0,
        {
            // Blending is enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
            // disabled for normals
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateTransparentMeshShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::LESS_OR_EQUAL;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = true;

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/TransparentMesh.program",
        {"shaders/mesh.vert", "shaders/mesh_transparent.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/mesh.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/mesh.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::COUNTER_CLOCKWISE,
        0,
        {
            // Blending is enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
            // disabled for normals
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateAmbientOcclusionProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/ambientOcclusion.program",
        {"shaders/passthrough.vert", "shaders/ambientOcclusion.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>(
            "_Primitives/PipelineLayouts/ambientOcclusion.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/ssao.pass"),
        CullMode::NONE,
        FrontFace::CLOCKWISE,
        0,
        {
            // Blending enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateAmbientOcclusionBlurProgram()
{

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/ambientOcclusionBlur.program",
        {"shaders/passthrough.vert", "shaders/ambientOcclusionBlur.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>(
            "_Primitives/PipelineLayouts/ambientOcclusionBlur.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::NONE,
        FrontFace::CLOCKWISE,
        0,
        {
            // Blending enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateParticleRenderingProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::LESS_OR_EQUAL;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = false; // No depth write so we don't get SSAO on particles
    // Constant ID 1 = isWorldspaceParticleEmitter = false
    std::unordered_map<uint32_t, uint32_t> localSpaceSpecialization{{1, 0}};
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/particleRendering.program",
        {"shaders/particles.vert", "shaders/particles.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/particles.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/particles.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        0,
        {// Blending enabled for color attachment
         {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
         // Does not write to normals buffer - don't blend?
         {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::ATTACHMENT_DISABLED}},
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil,
        localSpaceSpecialization);
    // Constant ID 1 = isWorldspaceParticleEmitter = true
    std::unordered_map<uint32_t, uint32_t> worldSpaceSpecialization{{1, 1}};
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/particleRendering_worldSpace.program",
        {"shaders/particles.vert", "shaders/particles.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/particles.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/particles.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        0,
        {// Blending enabled for color attachment
         {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
         // Does not write to normals buffer - don't blend?
         {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::ATTACHMENT_DISABLED}},
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil,
        // Constant ID 1 = isWorldspaceParticleEmitter = true
        worldSpaceSpecialization);
}

void ShaderProgramFactory::CreateTonemapProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/tonemap.program",
        {"shaders/passthrough.vert", "shaders/tonemap.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/tonemap.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::NONE,
        FrontFace::CLOCKWISE,
        1,
        {
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}

void ShaderProgramFactory::CreateUiShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/ui.program",
        {"shaders/ui.vert", "shaders/ui.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/ui.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/ui.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::NONE,
        FrontFace::COUNTER_CLOCKWISE,
        1,
        {
            // Blending enabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}
void ShaderProgramFactory::CreatePostprocessShaderProgram()
{
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthCompareOp = CompareOp::ALWAYS;
    depthStencil.depthTestEnable = false;
    depthStencil.depthWriteEnable = false;

    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/postprocess.program",
        {"shaders/passthrough.vert", "shaders/passthrough.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        1,
        {
            // Blending disabled for color attachment
            {.blendMode = ResourceCreationContext::GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}
