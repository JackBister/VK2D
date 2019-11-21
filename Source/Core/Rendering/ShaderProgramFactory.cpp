#include "ShaderProgramFactory.h"

#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"

void ShaderProgramFactory::CreateResources()
{
    CreatePassthroughTransformShaderProgram();
    CreateUiShaderProgram();
    CreatePostprocessShaderProgram();
}

void ShaderProgramFactory::CreatePassthroughTransformShaderProgram()
{
#if BAKE_SHADERS
    std::vector<ShaderProgram::ShaderStageCreateInfo> stages = {
        {"shaders/passthrough-transform.vert",
         ResourceManager::GetResource<ShaderModuleHandle>("shaders/passthrough-transform.vert")},
        {"shaders/passthrough-transform.frag",
         ResourceManager::GetResource<ShaderModuleHandle>("shaders/passthrough-transform.frag")}};

    // TODO:
    new ShaderProgram("_Primitives/ShaderPrograms/passthrough-transform.program",
                      ResourceManager::GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe"),
                      stages);
#else
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/passthrough-transform.program",
        {"shaders/passthrough-transform.vert", "shaders/passthrough-transform.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/pt.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        0);
#endif
}
void ShaderProgramFactory::CreateUiShaderProgram()
{
#if BAKE_SHADERS
    std::vector<ShaderProgram::ShaderStageCreateInfo> stages = {
        {"shaders/ui.vert", ResourceManager::GetResource<ShaderModuleHandle>("shaders/ui.vert")},
        {"shaders/ui.frag", ResourceManager::GetResource<ShaderModuleHandle>("shaders/ui.frag")}};

    new ShaderProgram("_Primitives/ShaderPrograms/ui.program",
                      ResourceManager::GetResource<PipelineHandle>("_Primitives/Pipelines/ui.pipe"),
                      stages);
#else
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/ui.program",
        {"shaders/ui.vert", "shaders/ui.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/ui.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/ui.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/main.pass"),
        CullMode::NONE,
        FrontFace::COUNTER_CLOCKWISE,
        0);
#endif
}
void ShaderProgramFactory::CreatePostprocessShaderProgram()
{

#if BAKE_SHADERS
    std::vector<ShaderProgram::ShaderStageCreateInfo> stages = {
        {"shaders/passthrough.vert", ResourceManager::GetResource<ShaderModuleHandle>("shaders/passthrough.vert")},
        {"shaders/passthrough.frag", ResourceManager::GetResource<ShaderModuleHandle>("shaders/passthrough.frag")}};

    new ShaderProgram("_Primitives/ShaderPrograms/postprocess.program",
                      ResourceManager::GetResource<PipelineHandle>("_Primitives/Pipelines/postprocess.pipe"),
                      stages);
#else
    ShaderProgram::Create(
        "_Primitives/ShaderPrograms/postprocess.program",
        {"shaders/passthrough.vert", "shaders/passthrough.frag"},
        ResourceManager::GetResource<VertexInputStateHandle>(
            "_Primitives/VertexInputStates/passthrough-transform.state"),
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/postprocess.pipelinelayout"),
        ResourceManager::GetResource<RenderPassHandle>("_Primitives/Renderpasses/postprocess.pass"),
        CullMode::BACK,
        FrontFace::CLOCKWISE,
        0);
#endif
}
