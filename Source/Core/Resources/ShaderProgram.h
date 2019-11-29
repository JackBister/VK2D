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
                  std::vector<ShaderStageCreateInfo> stageCreateInfo, VertexInputStateHandle * vertexInputState,
                  PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass, CullMode cullMode,
                  FrontFace frontFace, uint32_t subpass);

    static ShaderProgram * Create(std::string const & name, std::vector<std::string> fileNames,
                                  VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout,
                                  RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace,
                                  uint32_t subpass);

    inline PipelineHandle * GetPipeline() { return pipeline; }

    void SetRenderpass(RenderPassHandle *);

private:
    struct ShaderStage {
        std::string fileName;
        ResourceCreationContext::ShaderModuleCreateInfo::Type stage;
        bool compiledSuccessfully;
        ShaderModuleHandle * shaderModule;
    };

    ShaderProgram(std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStage> & stages,
                  VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout,
                  RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace, uint32_t subpass);

    static std::vector<ShaderStage> ReadShaderStages(std::vector<std::string> const & fileNames,
                                                     ResourceCreationContext & ctx);
    static PipelineHandle * CreatePipeline(std::vector<ShaderStage>, VertexInputStateHandle * vertexInputState,
                                           PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
                                           CullMode cullMode, FrontFace frontFace, uint32_t subpass,
                                           ResourceCreationContext & ctx);

    std::string name;
    PipelineHandle * pipeline;

    std::vector<ShaderStage> stages;

    VertexInputStateHandle * vertexInputState;
    PipelineLayoutHandle * pipelineLayout;
    RenderPassHandle * renderPass;
    uint32_t subpass;
    CullMode cullMode;
    FrontFace frontFace;
};