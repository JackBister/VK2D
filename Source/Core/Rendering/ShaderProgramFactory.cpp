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
            {false},
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
            {false},
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
            {true},
            // disabled for normals
            {false},
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
            {true},
            // disabled for normals
            {false},
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
            {true},
            // disabled for normals
            {false},
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
            {true},
            // disabled for normals
            {false},
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
    ShaderProgram::Create("_Primitives/ShaderPrograms/ambientOcclusion.program",
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
                              {true},
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
    ShaderProgram::Create("_Primitives/ShaderPrograms/ambientOcclusionBlur.program",
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
                              {true},
                          },
                          {
                              PrimitiveTopology::TRIANGLE_LIST,
                          },
                          depthStencil);
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
            // Blending enabled for color attachment
            {false},
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
            {true},
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
            {false},
        },
        {
            PrimitiveTopology::TRIANGLE_LIST,
        },
        depthStencil);
}
