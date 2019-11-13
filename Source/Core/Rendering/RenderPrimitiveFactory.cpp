#include "RenderPrimitiveFactory.h"

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/ResourceManager.h"
#include "Core/Semaphore.h"

#include "Core/Rendering/Shaders/passthrough-transform.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough.vert.spv.h"
#include "Core/Rendering/Shaders/ui.frag.spv.h"
#include "Core/Rendering/Shaders/ui.vert.spv.h"

RenderPrimitiveFactory::RenderPrimitiveFactory(Renderer * renderer, ResourceManager * resourceManager)
    : renderer(renderer), resourceManager(resourceManager)
{
}

void RenderPrimitiveFactory::CreatePrimitives()
{
    Semaphore finishedCreating;
    renderer->CreateResources([this, &finishedCreating](ResourceCreationContext & ctx) {
        auto mainRenderpass = CreateMainRenderpass(ctx);
        auto postprocessRenderpass = CreatePostprocessRenderpass(ctx);

        auto passthroughTransformVertShader = CreatePassthroughTransformVertShader(ctx);
        auto passthroughTransformFragShader = CreatePassthroughTransformFragShader(ctx);
        CreatePassthroughTransformGraphicsPipeline(
            ctx, mainRenderpass, passthroughTransformVertShader, passthroughTransformFragShader);

        auto uiVertShader = CreateUiVertShader(ctx);
        auto uiFragShader = CreateUiFragShader(ctx);
        CreateUiGraphicsPipeline(ctx, mainRenderpass, uiVertShader, uiFragShader);

        auto passthroughNoTransformVertShader = CreatePassthroughNoTransformVertShader(ctx);
        auto passthroughNoTransformFragShader = CreatePassthroughNoTransformFragShader(ctx);
        CreatePostprocessGraphicsPipeline(
            ctx, postprocessRenderpass, passthroughNoTransformVertShader, passthroughNoTransformFragShader);

        CreateQuadEbo(ctx);
        CreateQuadVbo(ctx);

        finishedCreating.Signal();
    });
    finishedCreating.Wait();
}

RenderPassHandle * RenderPrimitiveFactory::CreateMainRenderpass(ResourceCreationContext & ctx)
{
    RenderPassHandle::AttachmentDescription attachment = {0,
                                                          renderer->GetBackbufferFormat(),
                                                          RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
                                                          RenderPassHandle::AttachmentDescription::StoreOp::STORE,
                                                          RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
                                                          RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
                                                          ImageLayout::UNDEFINED,
                                                          ImageLayout::PRESENT_SRC_KHR};

    RenderPassHandle::AttachmentReference reference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    RenderPassHandle::SubpassDescription subpass = {
        RenderPassHandle::PipelineBindPoint::GRAPHICS, 0, nullptr, 1, &reference, nullptr, nullptr, 0, nullptr};

    ResourceCreationContext::RenderPassCreateInfo passInfo = {1, &attachment, 1, &subpass, 0, nullptr};
    auto mainRenderpass = ctx.CreateRenderPass(passInfo);
    resourceManager->AddResource("_Primitives/Renderpasses/main.pass", mainRenderpass);
    return mainRenderpass;
}

RenderPassHandle * RenderPrimitiveFactory::CreatePostprocessRenderpass(ResourceCreationContext & ctx)
{
    RenderPassHandle::AttachmentDescription postprocessAttachment = {
        0,
        renderer->GetBackbufferFormat(),
        // TODO: Can probably be DONT_CARE once we're doing actual post processing
        RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
        RenderPassHandle::AttachmentDescription::StoreOp::STORE,
        RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
        RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
        ImageLayout::UNDEFINED,
        ImageLayout::PRESENT_SRC_KHR};

    RenderPassHandle::AttachmentReference postprocessReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    RenderPassHandle::SubpassDescription postprocessSubpass = {RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                               0,
                                                               nullptr,
                                                               1,
                                                               &postprocessReference,
                                                               nullptr,
                                                               nullptr,
                                                               0,
                                                               nullptr};

    ResourceCreationContext::RenderPassCreateInfo postprocessPassInfo = {
        1, &postprocessAttachment, 1, &postprocessSubpass, 0, nullptr};
    auto postprocessRenderpass = ctx.CreateRenderPass(postprocessPassInfo);
    resourceManager->AddResource("_Primitives/Renderpasses/postprocess.pass", postprocessRenderpass);
    return postprocessRenderpass;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughTransformVertShader(ResourceCreationContext & ctx)
{
    auto ptvShader = ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                                             shaders_passthrough_transform_vert_spv_len,
                                             shaders_passthrough_transform_vert_spv});

    resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
    return ptvShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughTransformFragShader(ResourceCreationContext & ctx)
{
    auto pfShader = ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                                            shaders_passthrough_transform_frag_spv_len,
                                            shaders_passthrough_transform_frag_spv});

    resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.frag", pfShader);
    return pfShader;
}

void RenderPrimitiveFactory::CreatePassthroughTransformGraphicsPipeline(ResourceCreationContext & ctx,
                                                                        RenderPassHandle * renderpass,
                                                                        ShaderModuleHandle * vert,
                                                                        ShaderModuleHandle * frag)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding cameraUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto cameraPtLayout = ctx.CreateDescriptorSetLayout({1, cameraUniformBindings});
    resourceManager->AddResource("_Primitives/DescriptorSetLayouts/cameraPt.layout", cameraPtLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding spriteUniformBindings[2] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
        {1, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto spritePtLayout = ctx.CreateDescriptorSetLayout({2, spriteUniformBindings});
    resourceManager->AddResource("_Primitives/DescriptorSetLayouts/spritePt.layout", spritePtLayout);

    auto ptPipelineLayout = ctx.CreatePipelineLayout({{cameraPtLayout, spritePtLayout}});
    resourceManager->AddResource("_Primitives/PipelineLayouts/pt.pipelinelayout", ptPipelineLayout);

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "Passthrough-Transform Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "Passthrough-Transform Fragment Shader"}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, 8 * sizeof(float)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 3, false, 0},
        {0, 1, VertexComponentType::FLOAT, 3, false, 3 * sizeof(float)},
        {0, 2, VertexComponentType::FLOAT, 2, false, 6 * sizeof(float)}};
    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = attributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = binding;
    auto ptInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    resourceManager->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo ptRasterization{
        CullMode::BACK, FrontFace::CLOCKWISE};

    auto passthroughTransformPipeline =
        ctx.CreateGraphicsPipeline({2, stages, ptInputState, &ptRasterization, ptPipelineLayout, renderpass, 0});

    resourceManager->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", passthroughTransformPipeline);
}

ShaderModuleHandle * RenderPrimitiveFactory::CreateUiVertShader(ResourceCreationContext & ctx)
{
    auto uivShader = ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                                             shaders_ui_vert_spv_len,
                                             shaders_ui_vert_spv});

    resourceManager->AddResource("_Primitives/Shaders/ui.vert", uivShader);
    return uivShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreateUiFragShader(ResourceCreationContext & ctx)
{
    auto uifShader = ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                                             shaders_ui_frag_spv_len,
                                             shaders_ui_frag_spv});

    resourceManager->AddResource("_Primitives/Shaders/ui.frag", uifShader);
    return uifShader;
}

void RenderPrimitiveFactory::CreateUiGraphicsPipeline(ResourceCreationContext & ctx, RenderPassHandle * renderpass,
                                                      ShaderModuleHandle * vert, ShaderModuleHandle * frag)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uiBindings[1] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

    auto uiPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({1, uiBindings});
    resourceManager->AddResource("_Primitives/DescriptorSetLayouts/ui.layout", uiPipelineDescriptorSetLayout);

    auto uiLayout = ctx.CreatePipelineLayout({{uiPipelineDescriptorSetLayout}});
    resourceManager->AddResource("_Primitives/PipelineLayouts/ui.pipelinelayout", uiLayout);

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo uiStages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "UI Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "UI Fragment Shader"}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> uiBinding = {
        {0, 4 * sizeof(float) + 4 * sizeof(uint8_t)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> uiAttributes = {
        {0, 0, VertexComponentType::FLOAT, 2, false, 0},
        {0, 1, VertexComponentType::FLOAT, 2, false, 2 * sizeof(float)},
        {0, 2, VertexComponentType::UBYTE, 4, true, 4 * sizeof(float)}};

    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = uiAttributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = uiBinding;
    auto uiInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    resourceManager->AddResource("_Primitives/VertexInputStates/ui.state", uiInputState);

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo uiRasterization{
        CullMode::NONE, FrontFace::COUNTER_CLOCKWISE};

    auto uiPipeline =
        ctx.CreateGraphicsPipeline({2, uiStages, uiInputState, &uiRasterization, uiLayout, renderpass, 0});
    resourceManager->AddResource("_Primitives/Pipelines/ui.pipe", uiPipeline);
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughNoTransformVertShader(ResourceCreationContext & ctx)
{
    auto passthroughNoTransformVertShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
                                shaders_passthrough_vert_spv_len,
                                shaders_passthrough_vert_spv});

    resourceManager->AddResource("_Primitives/Shaders/passthrough.vert", passthroughNoTransformVertShader);
    return passthroughNoTransformVertShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughNoTransformFragShader(ResourceCreationContext & ctx)
{
    auto passthroughNoTransformFragShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
                                shaders_passthrough_frag_spv_len,
                                shaders_passthrough_frag_spv});

    resourceManager->AddResource("_Primitives/Shaders/passthrough.frag", passthroughNoTransformFragShader);
    return passthroughNoTransformFragShader;
}

void RenderPrimitiveFactory::CreatePostprocessGraphicsPipeline(ResourceCreationContext & ctx,
                                                               RenderPassHandle * renderpass, ShaderModuleHandle * vert,
                                                               ShaderModuleHandle * frag)
{
    // Create postprocess pipeline
    ResourceCreationContext::SamplerCreateInfo postprocessSamplerCreateInfo;
    postprocessSamplerCreateInfo.addressModeU = AddressMode::CLAMP_TO_EDGE;
    postprocessSamplerCreateInfo.addressModeV = AddressMode::CLAMP_TO_EDGE;
    postprocessSamplerCreateInfo.magFilter = Filter::NEAREST;
    postprocessSamplerCreateInfo.minFilter = Filter::NEAREST;
    auto postprocessSampler = ctx.CreateSampler(postprocessSamplerCreateInfo);
    resourceManager->AddResource("_Primitives/Samplers/postprocess.sampler", postprocessSampler);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding postprocessBindings[1] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

    auto postprocessDescriptorSetLayout = ctx.CreateDescriptorSetLayout({1, postprocessBindings});
    resourceManager->AddResource("_Primitives/DescriptorSetLayouts/postprocess.layout", postprocessDescriptorSetLayout);

    auto postprocessLayout = ctx.CreatePipelineLayout({{postprocessDescriptorSetLayout}});
    resourceManager->AddResource("_Primitives/PipelineLayouts/postprocess.pipelinelayout", postprocessLayout);

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo postprocessStages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "Passthrough-No Transform Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "Passthrough-No Transform Fragment Shader"}};

    auto ptInputState = resourceManager->GetResource<VertexInputStateHandle>(
        "_Primitives/VertexInputStates/passthrough-transform.state");

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo ptRasterization{
        CullMode::BACK, FrontFace::CLOCKWISE};

    auto postprocessPipeline = ctx.CreateGraphicsPipeline(
        {2, postprocessStages, ptInputState, &ptRasterization, postprocessLayout, renderpass, 0});
    resourceManager->AddResource("_Primitives/Pipelines/postprocess.pipe", postprocessPipeline);
}

void RenderPrimitiveFactory::CreateQuadEbo(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> const plainQuadElems = {0, 1, 2, 2, 3, 0};
    auto quadEbo = ctx.CreateBuffer({6 * sizeof(uint32_t),
                                     BufferUsageFlags::INDEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                     DEVICE_LOCAL_BIT});

    ctx.BufferSubData(quadEbo, (uint8_t *)&plainQuadElems[0], 0, plainQuadElems.size() * sizeof(uint32_t));

    resourceManager->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadEbo);
}

void RenderPrimitiveFactory::CreateQuadVbo(ResourceCreationContext & ctx)
{
    std::vector<float> const plainQuadVerts{
        // vec3 pos, vec3 color, vec2 texcoord
        -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
        1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
    };
    auto quadVbo = ctx.CreateBuffer({// 4 verts, 8 floats (vec3 pos, vec3 color, vec2 texcoord)
                                     8 * 4 * sizeof(float),
                                     BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                     DEVICE_LOCAL_BIT});

    ctx.BufferSubData(quadVbo, (uint8_t *)&plainQuadVerts[0], 0, plainQuadVerts.size() * sizeof(float));

    resourceManager->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVbo);
}