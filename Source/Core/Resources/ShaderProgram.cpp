#include "ShaderProgram.h"

#include <cassert>
#include <filesystem>

#include "Core/Rendering/GlslToSpirvShaderCompiler.h"
#include "Core/Resources/ResourceManager.h"
#include "Logging/Logger.h"
#include "Util/DefaultFileSlurper.h"
#include "Util/Semaphore.h"
#include "Util/WatchFile.h"

static const auto logger = Logger::Create("ShaderProgram");

static ResourceCreationContext::ShaderModuleCreateInfo::Type GetShaderStage(std::string const & fileName)
{
    auto extension = std::filesystem::path(fileName).extension().string();
    if (extension == ".vert") {
        return ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER;
    } else if (extension == ".frag") {
        return ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER;
    } else {
        logger.Error("Unknown shader file extension '{}'", extension);
        assert(false);
        return ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER;
    }
}

static ShaderStageFlagBits ToFlagBit(ResourceCreationContext::ShaderModuleCreateInfo::Type stage)
{
    switch (stage) {
    case ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER:
        return ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
    case ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER:
        return ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT;
    default:
        logger.Error("Unknown shader stage {}", stage);
        assert(false);
        return ShaderStageFlagBits::SHADER_STAGE_ALL_GRAPHICS;
    }
}

std::vector<ShaderProgram::ShaderStage> ShaderProgram::ReadShaderStages(std::vector<std::string> const & fileNames,
                                                                        ResourceCreationContext & ctx)
{
    GlslToSpirvShaderCompiler glslCompiler(std::make_shared<DefaultFileSlurper>());
    std::vector<ShaderStage> stages(fileNames.size());
    for (size_t i = 0; i < fileNames.size(); ++i) {
        stages[i].stage = GetShaderStage(fileNames[i]);
        stages[i].fileName = fileNames[i];
        auto compileResult = glslCompiler.CompileGlslFile(fileNames[i]);
        if (!compileResult.IsSuccessful()) {
            logger.Error("GLSL compilation failed, error message: {}", compileResult.GetErrorMessage().value());
            stages[i].compiledSuccessfully = false;
            continue;
        }
        stages[i].compiledSuccessfully = true;

        ResourceCreationContext::ShaderModuleCreateInfo shaderCreateInfo;
        shaderCreateInfo.code = compileResult.GetCompiledCode();
        shaderCreateInfo.type = stages[i].stage;
        stages[i].shaderModule = ctx.CreateShaderModule(shaderCreateInfo);
    }
    return stages;
}

PipelineHandle * ShaderProgram::CreatePipeline(
    std::vector<ShaderStage> stages, VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout,
    RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace, uint32_t subpass,
    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
    std::unordered_map<uint32_t, uint32_t> specializationConstants, ResourceCreationContext & ctx)
{

    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo> stagesCreateInfo(
        stages.size());
    for (size_t i = 0; i < stages.size(); ++i) {
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stageCreateInfo;
        stageCreateInfo.name = stages[i].fileName;
        stageCreateInfo.module = stages[i].shaderModule;
        stageCreateInfo.stage = ToFlagBit(stages[i].stage);
        stagesCreateInfo[i] = stageCreateInfo;
    }

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo rasterizationInfo;
    rasterizationInfo.cullMode = cullMode;
    rasterizationInfo.frontFace = frontFace;

    ResourceCreationContext::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.specializationConstants = specializationConstants;
    pipelineCreateInfo.depthStencil = &depthStencil;
    pipelineCreateInfo.pipelineLayout = pipelineLayout;
    pipelineCreateInfo.rasterizationState = &rasterizationInfo;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.stageCount = stagesCreateInfo.size();
    pipelineCreateInfo.pStages = stagesCreateInfo.data();
    pipelineCreateInfo.subpass = subpass;
    pipelineCreateInfo.colorBlendAttachments = colorBlendAttachments;
    pipelineCreateInfo.vertexInputState = vertexInputState;
    pipelineCreateInfo.inputAssembly = inputAssembly;
    return ctx.CreateGraphicsPipeline(pipelineCreateInfo);
}

ShaderProgram::ShaderProgram(
    std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStageCreateInfo> stageCreateInfo,
    VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
    CullMode cullMode, FrontFace frontFace, uint32_t subpass,
    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil)
    : name(name), pipeline(pipeline), vertexInputState(vertexInputState), pipelineLayout(pipelineLayout),
      renderPass(renderPass), cullMode(cullMode), frontFace(frontFace), subpass(subpass),
      colorBlendAttachments(colorBlendAttachments), depthStencil(depthStencil)
{
    for (auto const & ci : stageCreateInfo) {
        ShaderStage stage;
        stage.compiledSuccessfully = true;
        stage.fileName = ci.fileName;
        stage.shaderModule = ci.shaderModule;
        stage.stage = GetShaderStage(ci.fileName);
        stages.push_back(stage);
    }
    ResourceManager::AddResource(name, this);
}

ShaderProgram * ShaderProgram::Create(
    std::string const & name, std::vector<std::string> fileNames, VertexInputStateHandle * vertexInputState,
    PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace,
    uint32_t subpass,
    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
    std::unordered_map<uint32_t, uint32_t> specializationConstants)
{
    assert(fileNames.size() > 0);

    ShaderProgram * ret;
    Semaphore sem;
    ResourceManager::CreateResources([&ret,
                                      &sem,
                                      depthStencil,
                                      name,
                                      fileNames,
                                      cullMode,
                                      frontFace,
                                      pipelineLayout,
                                      renderPass,
                                      subpass,
                                      colorBlendAttachments,
                                      inputAssembly,
                                      vertexInputState,
                                      specializationConstants](ResourceCreationContext & ctx) {
        GlslToSpirvShaderCompiler glslCompiler(std::make_shared<DefaultFileSlurper>());

        auto stages = ReadShaderStages(fileNames, ctx);
        for (size_t i = 0; i < stages.size(); ++i) {
            auto const & stage = stages[i];
            if (!stage.compiledSuccessfully) {
                logger.Error("Shader stage {} ({}) failed to compile for shader program '{}', will return null.",
                             i,
                             stage.fileName,
                             name);
                ret = nullptr;
                sem.Signal();
                return;
            }
        }
        auto pipeline = CreatePipeline(stages,
                                       vertexInputState,
                                       pipelineLayout,
                                       renderPass,
                                       cullMode,
                                       frontFace,
                                       subpass,
                                       colorBlendAttachments,
                                       inputAssembly,
                                       depthStencil,
                                       specializationConstants,
                                       ctx);

        ret = new ShaderProgram(name,
                                pipeline,
                                stages,
                                vertexInputState,
                                pipelineLayout,
                                renderPass,
                                cullMode,
                                frontFace,
                                subpass,
                                colorBlendAttachments,
                                inputAssembly,
                                depthStencil,
                                specializationConstants);
        sem.Signal();
    });
    sem.Wait();
    ResourceManager::AddResource(name, ret);

#if HOT_RELOAD_RESOURCES
    for (auto fileName : fileNames) {
        WatchFile(fileName, [name, fileName]() {
            logger.Info("fileName='{}' changed, will reload program '{}'", fileName, name);
            auto program = ResourceManager::GetResource<ShaderProgram>(name);
            if (!program) {
                logger.Warn(
                    "fileName='{}' changed, but ResourceManager had no reference to program '{}'", fileName, name);
                return;
            }
            ResourceManager::CreateResources([program](ResourceCreationContext & ctx) {
                std::vector<std::string> fileNames(program->stages.size());
                for (size_t i = 0; i < fileNames.size(); ++i) {
                    fileNames[i] = program->stages[i].fileName;
                }
                auto oldStages = program->stages;
                auto newStages = ReadShaderStages(fileNames, ctx);
                for (size_t i = 0; i < newStages.size(); ++i) {
                    if (!newStages[i].compiledSuccessfully) {
                        logger.Error("Shader stage {} ({}) failed to compile for shader program '{}', will not reload.",
                                     i + 1,
                                     newStages[i].fileName,
                                     program->name);
                        ResourceManager::DestroyResources([newStages](ResourceCreationContext & ctx) {
                            for (auto const & stage : newStages) {
                                if (stage.compiledSuccessfully) {
                                    ctx.DestroyShaderModule(stage.shaderModule);
                                }
                            }
                        });

                        return;
                    }
                }
                auto oldPipeline = program->pipeline;
                program->stages = newStages;
                program->pipeline = CreatePipeline(program->stages,
                                                   program->vertexInputState,
                                                   program->pipelineLayout,
                                                   program->renderPass,
                                                   program->cullMode,
                                                   program->frontFace,
                                                   program->subpass,
                                                   program->colorBlendAttachments,
                                                   program->inputAssembly,
                                                   program->depthStencil,
                                                   program->specializationConstants,
                                                   ctx);
                ResourceManager::DestroyResources([oldStages, oldPipeline](ResourceCreationContext & ctx) {
                    ctx.DestroyPipeline(oldPipeline);
                    for (auto const & stage : oldStages) {
                        ctx.DestroyShaderModule(stage.shaderModule);
                    }
                });
            });
        });
    }
#endif

    return ret;
}

void ShaderProgram::SetRenderpass(RenderPassHandle * newPass)
{
    ResourceManager::CreateResources([this, newPass](ResourceCreationContext & ctx) {
        auto oldPipeline = this->pipeline;
        this->renderPass = newPass;
        this->pipeline = CreatePipeline(this->stages,
                                        this->vertexInputState,
                                        this->pipelineLayout,
                                        this->renderPass,
                                        this->cullMode,
                                        this->frontFace,
                                        this->subpass,
                                        this->colorBlendAttachments,
                                        this->inputAssembly,
                                        this->depthStencil,
                                        this->specializationConstants,
                                        ctx);
        ResourceManager::DestroyResources(
            [oldPipeline](ResourceCreationContext & ctx) { ctx.DestroyPipeline(oldPipeline); });
    });
}

ShaderProgram::ShaderProgram(
    std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStage> & stages,
    VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
    CullMode cullMode, FrontFace frontFace, uint32_t subpass,
    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
    std::unordered_map<uint32_t, uint32_t> specializationConstants)
    : name(name), pipeline(pipeline), stages(stages), vertexInputState(vertexInputState),
      pipelineLayout(pipelineLayout), renderPass(renderPass), cullMode(cullMode), frontFace(frontFace),
      subpass(subpass), colorBlendAttachments(colorBlendAttachments), inputAssembly(inputAssembly),
      depthStencil(depthStencil), specializationConstants(specializationConstants)
{
}
