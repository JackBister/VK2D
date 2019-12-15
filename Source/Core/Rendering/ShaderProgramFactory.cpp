#include "ShaderProgramFactory.h"

#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"

void ShaderProgramFactory::CreateResources()
{
    CreatePassthroughTransformShaderProgram();
    CreateMeshShaderProgram();
    CreateTransparentMeshShaderProgram();
    CreateUiShaderProgram();
    CreatePostprocessShaderProgram();
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
        0,
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
        0,
        depthStencil);
}
