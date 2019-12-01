#include "RenderPrimitiveFactory.h"

#include <imgui.h>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMesh.h"
#include "Core/Semaphore.h"

#if BAKE_SHADERS
#include "Core/Rendering/Shaders/passthrough-transform.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough.vert.spv.h"
#include "Core/Rendering/Shaders/ui.frag.spv.h"
#include "Core/Rendering/Shaders/ui.vert.spv.h"
#endif

RenderPrimitiveFactory::RenderPrimitiveFactory(Renderer * renderer) : renderer(renderer) {}

void RenderPrimitiveFactory::CreatePrimitives()
{
    Semaphore finishedCreating;
    renderer->CreateResources([this, &finishedCreating](ResourceCreationContext & ctx) {
        auto mainRenderpass = CreateMainRenderpass(ctx);
        auto postprocessRenderpass = CreatePostprocessRenderpass(ctx);

        auto passthroughTransformPipelineLayout = CreatePassthroughTransformPipelineLayout(ctx);
        auto passthroughTransformVertexInputState = CreatePassthroughTransformVertexInputState(ctx);

        CreateMeshPipelineLayout(ctx);
        CreateMeshVertexInputState(ctx);

        auto uiPipelineLayout = CreateUiPipelineLayout(ctx);
        auto uiVertexInputState = CreateUiVertexInputState(ctx);

        auto postprocessPipelineLayout = CreatePostprocessPipelineLayout(ctx);

#if BAKE_SHADERS
        auto passthroughTransformVertShader = CreatePassthroughTransformVertShader(ctx);
        auto passthroughTransformFragShader = CreatePassthroughTransformFragShader(ctx);
        CreatePassthroughTransformGraphicsPipeline(ctx,
                                                   mainRenderpass,
                                                   passthroughTransformPipelineLayout,
                                                   passthroughTransformVertexInputState,
                                                   passthroughTransformVertShader,
                                                   passthroughTransformFragShader);

        auto uiVertShader = CreateUiVertShader(ctx);
        auto uiFragShader = CreateUiFragShader(ctx);
        CreateUiGraphicsPipeline(ctx, mainRenderpass, uiPipelineLayout, uiVertexInputState, uiVertShader, uiFragShader);

        auto passthroughNoTransformVertShader = CreatePassthroughNoTransformVertShader(ctx);
        auto passthroughNoTransformFragShader = CreatePassthroughNoTransformFragShader(ctx);
        CreatePostprocessGraphicsPipeline(ctx,
                                          postprocessRenderpass,
                                          postprocessPipelineLayout,
                                          passthroughNoTransformVertShader,
                                          passthroughNoTransformFragShader);
#endif

        CreateDefaultSampler(ctx);

        CreateFontImage(ctx);

        CreateQuadEbo(ctx);
        CreateQuadVbo(ctx);

        CreateBoxVbo(ctx);

        finishedCreating.Signal();
    });
    finishedCreating.Wait();
}

void RenderPrimitiveFactory::LateCreatePrimitives()
{
    ResourceManager::CreateResources([this](ResourceCreationContext & ctx) {
        CreateDefaultMaterial(ctx);
        CreateQuadMesh();
        CreateBoxMesh();
    });
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
                                                          ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    RenderPassHandle::AttachmentReference reference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    RenderPassHandle::SubpassDescription subpass = {
        RenderPassHandle::PipelineBindPoint::GRAPHICS, 0, nullptr, 1, &reference, nullptr, nullptr, 0, nullptr};

    ResourceCreationContext::RenderPassCreateInfo passInfo = {1, &attachment, 1, &subpass, 0, nullptr};
    auto mainRenderpass = ctx.CreateRenderPass(passInfo);
    ResourceManager::AddResource("_Primitives/Renderpasses/main.pass", mainRenderpass);
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
        ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
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
    ResourceManager::AddResource("_Primitives/Renderpasses/postprocess.pass", postprocessRenderpass);
    return postprocessRenderpass;
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreatePassthroughTransformPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding cameraUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto cameraPtLayout = ctx.CreateDescriptorSetLayout({1, cameraUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/cameraPt.layout", cameraPtLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding spriteUniformBindings[2] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
        {1, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto spritePtLayout = ctx.CreateDescriptorSetLayout({2, spriteUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/spritePt.layout", spritePtLayout);

    auto ptPipelineLayout = ctx.CreatePipelineLayout({{cameraPtLayout, spritePtLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/pt.pipelinelayout", ptPipelineLayout);

    return ptPipelineLayout;
}

VertexInputStateHandle *
RenderPrimitiveFactory::CreatePassthroughTransformVertexInputState(ResourceCreationContext & ctx)
{
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
    ResourceManager::AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);

    return ptInputState;
}

void RenderPrimitiveFactory::CreateMeshPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding cameraUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto cameraMeshLayout = ctx.CreateDescriptorSetLayout({1, cameraUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/cameraMesh.layout", cameraMeshLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding modelUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto modelMeshLayout = ctx.CreateDescriptorSetLayout({1, modelUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/modelMesh.layout", modelMeshLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding materialUniformBindings[1] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto materialMeshLayout = ctx.CreateDescriptorSetLayout({1, materialUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/materialMesh.layout", materialMeshLayout);

    auto meshPipelineLayout = ctx.CreatePipelineLayout({{cameraMeshLayout, modelMeshLayout, materialMeshLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/mesh.pipelinelayout", meshPipelineLayout);
}

void RenderPrimitiveFactory::CreateMeshVertexInputState(ResourceCreationContext & ctx)
{
    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, 8 * sizeof(float)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 3, false, 0},
        {0, 1, VertexComponentType::FLOAT, 3, false, 3 * sizeof(float)},
        {0, 2, VertexComponentType::FLOAT, 2, false, 6 * sizeof(float)}};
    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = attributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = binding;
    auto meshInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    ResourceManager::AddResource("_Primitives/VertexInputStates/mesh.state", meshInputState);
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreateUiPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding fragBindings[] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
        {1, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto uiPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({2, fragBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/ui.layout", uiPipelineDescriptorSetLayout);

    auto uiLayout = ctx.CreatePipelineLayout({{uiPipelineDescriptorSetLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/ui.pipelinelayout", uiLayout);
    return uiLayout;
}

VertexInputStateHandle * RenderPrimitiveFactory::CreateUiVertexInputState(ResourceCreationContext & ctx)
{
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
    ResourceManager::AddResource("_Primitives/VertexInputStates/ui.state", uiInputState);
    return uiInputState;
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreatePostprocessPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding postprocessBindings[1] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};

    auto postprocessDescriptorSetLayout = ctx.CreateDescriptorSetLayout({1, postprocessBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/postprocess.layout", postprocessDescriptorSetLayout);

    auto postprocessLayout = ctx.CreatePipelineLayout({{postprocessDescriptorSetLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/postprocess.pipelinelayout", postprocessLayout);
    return postprocessLayout;
}

#if BAKE_SHADERS
ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughTransformVertShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code(
        (uint32_t *)shaders_passthrough_transform_vert_spv,
        (uint32_t *)(shaders_passthrough_transform_vert_spv + shaders_passthrough_transform_vert_spv_len));
    auto ptvShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER, code});

    ResourceManager::AddResource("shaders/passthrough-transform.vert", ptvShader);
    return ptvShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughTransformFragShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code(
        (uint32_t *)shaders_passthrough_transform_frag_spv,
        (uint32_t *)(shaders_passthrough_transform_frag_spv + shaders_passthrough_transform_frag_spv_len));
    auto pfShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER, code});

    ResourceManager::AddResource("shaders/passthrough-transform.frag", pfShader);
    return pfShader;
}

void RenderPrimitiveFactory::CreatePassthroughTransformGraphicsPipeline(
    ResourceCreationContext & ctx, RenderPassHandle * renderpass, PipelineLayoutHandle * layout,
    VertexInputStateHandle * vertexInputState, ShaderModuleHandle * vert, ShaderModuleHandle * frag)
{

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "Passthrough-Transform Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "Passthrough-Transform Fragment Shader"}};

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo ptRasterization{
        CullMode::BACK, FrontFace::CLOCKWISE};

    auto passthroughTransformPipeline =
        ctx.CreateGraphicsPipeline({2, stages, vertexInputState, &ptRasterization, layout, renderpass, 0});

    ResourceManager::AddResource("_Primitives/Pipelines/passthrough-transform.pipe", passthroughTransformPipeline);
}

ShaderModuleHandle * RenderPrimitiveFactory::CreateUiVertShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code((uint32_t *)shaders_ui_vert_spv,
                               (uint32_t *)(shaders_ui_vert_spv + shaders_ui_vert_spv_len));
    auto uivShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER, code});

    ResourceManager::AddResource("shaders/ui.vert", uivShader);
    return uivShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreateUiFragShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code((uint32_t *)shaders_ui_frag_spv,
                               (uint32_t *)(shaders_ui_frag_spv + shaders_ui_frag_spv_len));
    auto uifShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER, code});

    ResourceManager::AddResource("shaders/ui.frag", uifShader);
    return uifShader;
}

void RenderPrimitiveFactory::CreateUiGraphicsPipeline(ResourceCreationContext & ctx, RenderPassHandle * renderpass,
                                                      PipelineLayoutHandle * layout,
                                                      VertexInputStateHandle * vertexInputState,
                                                      ShaderModuleHandle * vert, ShaderModuleHandle * frag)
{

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo uiStages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "UI Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "UI Fragment Shader"}};

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo uiRasterization{
        CullMode::NONE, FrontFace::COUNTER_CLOCKWISE};

    auto uiPipeline =
        ctx.CreateGraphicsPipeline({2, uiStages, vertexInputState, &uiRasterization, layout, renderpass, 0});
    ResourceManager::AddResource("_Primitives/Pipelines/ui.pipe", uiPipeline);
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughNoTransformVertShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code((uint32_t *)shaders_passthrough_vert_spv,
                               (uint32_t *)(shaders_passthrough_vert_spv + shaders_passthrough_vert_spv_len));
    auto passthroughNoTransformVertShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER, code});

    ResourceManager::AddResource("shaders/passthrough.vert", passthroughNoTransformVertShader);
    return passthroughNoTransformVertShader;
}

ShaderModuleHandle * RenderPrimitiveFactory::CreatePassthroughNoTransformFragShader(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> code((uint32_t *)shaders_passthrough_frag_spv,
                               (uint32_t *)(shaders_passthrough_frag_spv + shaders_passthrough_frag_spv_len));
    auto passthroughNoTransformFragShader =
        ctx.CreateShaderModule({ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER, code});

    ResourceManager::AddResource("shaders/passthrough.frag", passthroughNoTransformFragShader);
    return passthroughNoTransformFragShader;
}

void RenderPrimitiveFactory::CreatePostprocessGraphicsPipeline(ResourceCreationContext & ctx,
                                                               RenderPassHandle * renderpass,
                                                               PipelineLayoutHandle * layout, ShaderModuleHandle * vert,
                                                               ShaderModuleHandle * frag)
{
    // Create postprocess pipeline

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo postprocessStages[2] = {
        {ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, vert, "Passthrough-No Transform Vertex Shader"},
        {ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, frag, "Passthrough-No Transform Fragment Shader"}};

    auto ptInputState = ResourceManager::GetResource<VertexInputStateHandle>(
        "_Primitives/VertexInputStates/passthrough-transform.state");

    ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo ptRasterization{
        CullMode::BACK, FrontFace::CLOCKWISE};

    auto postprocessPipeline =
        ctx.CreateGraphicsPipeline({2, postprocessStages, ptInputState, &ptRasterization, layout, renderpass, 0});
    ResourceManager::AddResource("_Primitives/Pipelines/postprocess.pipe", postprocessPipeline);
}
#endif

void RenderPrimitiveFactory::CreateDefaultSampler(ResourceCreationContext & ctx)
{
    ResourceCreationContext::SamplerCreateInfo sc = {};
    sc.addressModeU = AddressMode::REPEAT;
    sc.addressModeV = AddressMode::REPEAT;
    sc.magFilter = Filter::NEAREST;
    sc.magFilter = Filter::NEAREST;
    auto sampler = ctx.CreateSampler(sc);
    ResourceManager::AddResource("_Primitives/Samplers/Default.sampler", sampler);
}

void CreatePostprocessSampler(ResourceCreationContext & ctx)
{
    ResourceCreationContext::SamplerCreateInfo postprocessSamplerCreateInfo;
    postprocessSamplerCreateInfo.addressModeU = AddressMode::CLAMP_TO_EDGE;
    postprocessSamplerCreateInfo.addressModeV = AddressMode::CLAMP_TO_EDGE;
    postprocessSamplerCreateInfo.magFilter = Filter::NEAREST;
    postprocessSamplerCreateInfo.minFilter = Filter::NEAREST;
    auto postprocessSampler = ctx.CreateSampler(postprocessSamplerCreateInfo);
    ResourceManager::AddResource("_Primitives/Samplers/postprocess.sampler", postprocessSampler);
}

void RenderPrimitiveFactory::CreateFontImage(ResourceCreationContext & ctx)
{
    auto & imguiIo = ImGui::GetIO();
    imguiIo.MouseDrawCursor = false;
    auto res = renderer->GetResolution();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
    uint8_t * fontPixels;
    int fontWidth, fontHeight, bytesPerPixel;
    imguiIo.Fonts->AddFontDefault();
    imguiIo.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight, &bytesPerPixel);
    std::vector<uint8_t> fontPixelVector(fontWidth * fontHeight * bytesPerPixel);
    memcpy(&fontPixelVector[0], fontPixels, fontWidth * fontHeight * bytesPerPixel);

    ResourceCreationContext::ImageCreateInfo fontCreateInfo;
    fontCreateInfo.depth = 1;
    fontCreateInfo.format = Format::RGBA8;
    fontCreateInfo.height = fontHeight;
    fontCreateInfo.width = fontWidth;
    fontCreateInfo.mipLevels = 1;
    fontCreateInfo.type = ImageHandle::Type::TYPE_2D;
    fontCreateInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto fontAtlas = ctx.CreateImage(fontCreateInfo);
    ctx.ImageData(fontAtlas, fontPixelVector);
    ResourceManager::AddResource("_Primitives/Images/FontAtlas.img", fontAtlas);

    imguiIo.Fonts->TexID = fontAtlas;

    ResourceCreationContext::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    viewCreateInfo.format = Format::RGBA8;
    viewCreateInfo.image = fontAtlas;
    viewCreateInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto fontAtlasView = ctx.CreateImageView(viewCreateInfo);
    ResourceManager::AddResource("_Primitives/Images/FontAtlas.img/defaultView", fontAtlasView);

    auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/ui.layout");
    auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");
}

void RenderPrimitiveFactory::CreateDefaultMaterial(ResourceCreationContext & ctx)
{
    ResourceCreationContext::ImageCreateInfo albedoInfo;
    albedoInfo.depth = 1;
    albedoInfo.format = Format::RGBA8;
    albedoInfo.height = 2;
    albedoInfo.width = 2;
    albedoInfo.mipLevels = 1;
    albedoInfo.type = ImageHandle::Type::TYPE_2D;
    albedoInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto albedoImg = ctx.CreateImage(albedoInfo);
    std::vector<uint8_t> albedoData = {
        // clang-format off
        0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0xFF,
        // clang-format on
    };
    ctx.ImageData(albedoImg, albedoData);
    ResourceCreationContext::ImageViewCreateInfo albedoViewInfo;
    albedoViewInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    albedoViewInfo.format = Format::RGBA8;
    albedoViewInfo.image = albedoImg;
    albedoViewInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    albedoViewInfo.subresourceRange.baseArrayLayer = 0;
    albedoViewInfo.subresourceRange.baseMipLevel = 0;
    albedoViewInfo.subresourceRange.layerCount = 1;
    albedoViewInfo.subresourceRange.levelCount = 1;
    albedoViewInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto albedoView = ctx.CreateImageView(albedoViewInfo);

    auto albedoImage = new Image("_Primitives/Materials/default.mtl/image", 2, 2, albedoImg, albedoView);
    ResourceManager::AddResource("_Primitives/Materials/default.mtl/image", albedoImage);

    auto defaultMaterial = new Material(albedoImage);
    ResourceManager::AddResource("_Primitives/Materials/default.mtl", defaultMaterial);
}

void RenderPrimitiveFactory::CreateQuadEbo(ResourceCreationContext & ctx)
{
    std::vector<uint32_t> const plainQuadElems = {0, 1, 2, 2, 3, 0};
    auto quadEbo = ctx.CreateBuffer({6 * sizeof(uint32_t),
                                     BufferUsageFlags::INDEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                     DEVICE_LOCAL_BIT});

    ctx.BufferSubData(quadEbo, (uint8_t *)&plainQuadElems[0], 0, plainQuadElems.size() * sizeof(uint32_t));

    ResourceManager::AddResource("_Primitives/Buffers/QuadEBO.buffer", quadEbo);
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

    ResourceManager::AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVbo);
}

void RenderPrimitiveFactory::CreateQuadMesh()
{
    auto indexBuffer = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");
    auto vertexBuffer = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");
    auto material = ResourceManager::GetResource<Material>("_Primitives/Materials/default.mtl");

    Submesh submesh("main", material, 6, indexBuffer, 4, vertexBuffer);
    auto staticMesh = new StaticMesh({submesh});
    ResourceManager::AddResource("/_Primitives/Meshes/Quad.obj", staticMesh);
}

void RenderPrimitiveFactory::CreateBoxVbo(ResourceCreationContext & ctx)
{
    std::vector<float> const boxVerts{
        // clang-format off
        // back
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f, //back bottom left
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f, //back top left
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f, //back top right
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f, //back top right
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f, //back bottom right
        -0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f, //back bottom left

		//front
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,

		//left
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
       
        // right
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
        
		//bottom
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,

        // top
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.0f, 1.0f,
        //clang-format on
    };
    auto boxVbo = ctx.CreateBuffer({// 8 floats per vert, 6 verts per face, 6 faces
                                    8 * 6 * 6 * sizeof(float),
                                    BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                    DEVICE_LOCAL_BIT});

    ctx.BufferSubData(boxVbo, (uint8_t *)&boxVerts[0], 0, boxVerts.size() * sizeof(float));

    ResourceManager::AddResource("_Primitives/Buffers/BoxVBO.buffer", boxVbo);
}

void RenderPrimitiveFactory::CreateBoxMesh()
{
    auto vertexBuffer = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/BoxVBO.buffer");
    auto material = ResourceManager::GetResource<Material>("_Primitives/Materials/default.mtl");

    Submesh submesh("main", material, 6 * 6, vertexBuffer);
    auto staticMesh = new StaticMesh({submesh});
    ResourceManager::AddResource("/_Primitives/Meshes/Box.obj", staticMesh);
}
