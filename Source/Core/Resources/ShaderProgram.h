#pragma once

#include <string>
#include <vector>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

class ShaderProgram
{
public:
    struct ShaderStageCreateInfo {
        std::string fileName;
        ShaderModuleHandle * shaderModule;
    };

    ShaderProgram(std::string const & name, PipelineHandle * pipeline,
                  std::vector<ShaderStageCreateInfo> stageCreateInfo);

    static ShaderProgram * Create(std::string const & name, std::vector<std::string> fileNames,
                                  VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout,
                                  RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace,
                                  uint32_t subpass);

    inline PipelineHandle * GetPipeline() { return pipeline; }

private:
    struct ShaderStage {
        std::string fileName;
        ResourceCreationContext::ShaderModuleCreateInfo::Type stage;
        bool compiledSuccessfully;
        ShaderModuleHandle * shaderModule;
    };

    ShaderProgram(std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStage> & stages);

    static std::vector<ShaderStage> ReadShaderStages(std::vector<std::string> const & fileNames,
                                                     ResourceCreationContext & ctx);
    static PipelineHandle * CreatePipeline(std::vector<ShaderStage>, VertexInputStateHandle * vertexInputState,
                                           PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
                                           CullMode cullMode, FrontFace frontFace, uint32_t subpass,
                                           ResourceCreationContext & ctx);

    std::string name;
    PipelineHandle * pipeline;
    std::vector<ShaderStage> stages;
};
