#pragma once

#include <string>
#include <vector>

#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Abstract/ResourceCreationContext.h"

class ShaderProgram
{
public:
    struct ShaderStageCreateInfo {
        std::string fileName;
        ShaderModuleHandle * shaderModule;
    };

    ShaderProgram(
        std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStageCreateInfo> stageCreateInfo,
        VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
        CullMode cullMode, FrontFace frontFace, uint32_t subpass,
        std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
        ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil);

    static ShaderProgram *
    Create(std::string const & name, std::vector<std::string> fileNames, VertexInputStateHandle * vertexInputState,
           PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace,
           uint32_t subpass,
           std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
           ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
           ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
           std::unordered_map<uint32_t, uint32_t> specializationConstants = {});

    inline PipelineHandle * GetPipeline() { return pipeline; }

    void SetRenderpass(RenderPassHandle *);

private:
    struct ShaderStage {
        std::string fileName;
        ResourceCreationContext::ShaderModuleCreateInfo::Type stage;
        bool compiledSuccessfully;
        ShaderModuleHandle * shaderModule;
    };

    ShaderProgram(
        std::string const & name, PipelineHandle * pipeline, std::vector<ShaderStage> & stages,
        VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout, RenderPassHandle * renderPass,
        CullMode cullMode, FrontFace frontFace, uint32_t subpass,
        std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
        ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
        std::unordered_map<uint32_t, uint32_t> specializationConstants);

    static std::vector<ShaderStage> ReadShaderStages(std::vector<std::string> const & fileNames,
                                                     ResourceCreationContext & ctx);
    static PipelineHandle * CreatePipeline(
        std::vector<ShaderStage>, VertexInputStateHandle * vertexInputState, PipelineLayoutHandle * pipelineLayout,
        RenderPassHandle * renderPass, CullMode cullMode, FrontFace frontFace, uint32_t subpass,
        std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments,
        ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly,
        ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil,
        std::unordered_map<uint32_t, uint32_t> specializationConstants, ResourceCreationContext & ctx);

    std::string name;
    PipelineHandle * pipeline;

    std::vector<ShaderStage> stages;

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineDepthStencilStateCreateInfo depthStencil;
    std::unordered_map<uint32_t, uint32_t> specializationConstants;
    std::vector<ResourceCreationContext::GraphicsPipelineCreateInfo::ColorBlendAttachment> colorBlendAttachments;
    ResourceCreationContext::GraphicsPipelineCreateInfo::InputAssembly inputAssembly;
    VertexInputStateHandle * vertexInputState;
    PipelineLayoutHandle * pipelineLayout;
    RenderPassHandle * renderPass;
    uint32_t subpass;
    CullMode cullMode;
    FrontFace frontFace;
};
