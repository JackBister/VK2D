#include "RenderPrimitiveFactory.h"

#include <ThirdParty/imgui/imgui.h>

#include "Core/Rendering/Vertex.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMesh.h"
#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Renderer.h"
#include "Util/Semaphore.h"

RenderPrimitiveFactory::RenderPrimitiveFactory(Renderer * renderer) : renderer(renderer) {}

void RenderPrimitiveFactory::CreatePrimitives()
{
    Semaphore finishedCreating;
    renderer->CreateResources([this, &finishedCreating](ResourceCreationContext & ctx) {
        auto mainRenderpass = CreateMainRenderpass(ctx);
        CreateSSAORenderpass(ctx);
        auto postprocessRenderpass = CreatePostprocessRenderpass(ctx);

        auto passthroughTransformPipelineLayout = CreatePassthroughTransformPipelineLayout(ctx);
        auto passthroughTransformVertexInputState = CreatePassthroughTransformVertexInputState(ctx);

        CreateMeshPipelineLayout(ctx);
        CreateMeshVertexInputState(ctx);

        CreateSkeletalMeshPipelineLayout(ctx);
        CreateSkeletalMeshVertexInputState(ctx);

        CreateParticlePipelineLayout(ctx);
        CreateParticleVertexInputState(ctx);

        CreateAmbientOcclusionPipelineLayout(ctx);
        CreateAmbientOcclusionBlurPipelineLayout(ctx);
        CreateTonemapPipelineLayout(ctx);
        auto uiPipelineLayout = CreateUiPipelineLayout(ctx);
        auto uiVertexInputState = CreateUiVertexInputState(ctx);

        auto postprocessPipelineLayout = CreatePostprocessPipelineLayout(ctx);

        CreatePrepassPipelineLayout(ctx);
        CreateSkeletalPrepassPipelineLayout(ctx);

        CreateDefaultSampler(ctx);
        CreatePostprocessSampler(ctx);

        CreateFontImage(ctx);

        CreateQuadEbo(ctx);
        CreateQuadVbo(ctx);

        CreateBoxVbo(ctx);

        CreateDebugDrawPointVertexInputState(ctx);
        CreateDebugDrawPipelineLayout(ctx);

        finishedCreating.Signal();
    });
    finishedCreating.Wait();
}

void RenderPrimitiveFactory::LateCreatePrimitives()
{
    ResourceManager::CreateResources([this](ResourceCreationContext & ctx) {
        CreateDefaultNormalMap(ctx);
        CreateDefaultRoughnessMap(ctx);
        CreateDefaultMetallicMap(ctx);
        CreateDefaultMaterial(ctx);
        CreateQuadMesh();
        CreateBoxMesh();
    });
}

void RenderPrimitiveFactory::CreateDebugDrawPointVertexInputState(ResourceCreationContext & ctx)
{
    ResourceCreationContext::VertexInputStateCreateInfo ci;

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, 2 * sizeof(glm::vec3)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 3, false, 0},
        {0, 1, VertexComponentType::FLOAT, 3, true, 3 * sizeof(float)},
    };

    ci.vertexBindingDescriptions = binding;
    ci.vertexAttributeDescriptions = attributes;

    auto inputState = ctx.CreateVertexInputState(ci);
    ResourceManager::AddResource("_Primitives/VertexInputStates/debug_draw.state", inputState);
}

void RenderPrimitiveFactory::CreateDebugDrawPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::PipelineLayoutCreateInfo ci;
    auto camera =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraPt.layout");
    ci.setLayouts = {camera};
    auto layout = ctx.CreatePipelineLayout(ci);
    ResourceManager::AddResource("_Primitives/PipelineLayouts/debug_draw.pipelinelayout", layout);
}

RenderPassHandle * RenderPrimitiveFactory::CreateMainRenderpass(ResourceCreationContext & ctx)
{
    std::vector<RenderPassHandle::AttachmentDescription> attachments = {
        // Main color buffer
        {
            0,
            Format::R32G32B32A32_SFLOAT,
            RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
            RenderPassHandle::AttachmentDescription::StoreOp::STORE,
            RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
            RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
            ImageLayout::UNDEFINED,
            ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        },
        // Normals buffer
        {
            0,
            Format::R16G16_SFLOAT,
            RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
            RenderPassHandle::AttachmentDescription::StoreOp::STORE,
            RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
            RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
            ImageLayout::UNDEFINED,
            ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        },
        // Depth
        {
            0,
            Format::D32_SFLOAT,
            RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
            RenderPassHandle::AttachmentDescription::StoreOp::STORE,
            RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
            RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
            ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        }};

    RenderPassHandle::AttachmentReference colorReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    RenderPassHandle::AttachmentReference normalsGBufferReference = {1, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    RenderPassHandle::AttachmentReference depthReference = {2, ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL};

    RenderPassHandle::SubpassDescription mainpassSubpass = {RenderPassHandle::PipelineBindPoint::GRAPHICS,
                                                            {},
                                                            {colorReference, normalsGBufferReference},
                                                            {},
                                                            depthReference,
                                                            {}};

    ResourceCreationContext::RenderPassCreateInfo passInfo = {attachments, {mainpassSubpass}, {}};
    auto mainRenderpass = ctx.CreateRenderPass(passInfo);
    ResourceManager::AddResource("_Primitives/Renderpasses/main.pass", mainRenderpass);
    return mainRenderpass;
}

RenderPassHandle * RenderPrimitiveFactory::CreateSSAORenderpass(ResourceCreationContext & ctx)
{
    std::vector<RenderPassHandle::AttachmentDescription> ssaoAttachments = {
        {0,
         Format::R32_SFLOAT,
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::STORE,
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
         ImageLayout::UNDEFINED,
         ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
    };
    RenderPassHandle::AttachmentReference outputReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    RenderPassHandle::SubpassDescription subpass = {
        RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {outputReference}, {}, {}, {}};
    ResourceCreationContext::RenderPassCreateInfo ci;
    ci.attachments = ssaoAttachments;
    ci.subpasses = {subpass};
    auto renderpass = ctx.CreateRenderPass(ci);
    ResourceManager::AddResource("_Primitives/Renderpasses/ssao.pass", renderpass);
    return renderpass;
}

RenderPassHandle * RenderPrimitiveFactory::CreatePostprocessRenderpass(ResourceCreationContext & ctx)
{
    std::vector<RenderPassHandle::AttachmentDescription> postprocessAttachments = {
        // Backbuffer
        {0,
         renderer->GetBackbufferFormat(),
         // TODO: Can probably be DONT_CARE once we're doing actual post processing
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::STORE,
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
         ImageLayout::UNDEFINED,
         ImageLayout::PRESENT_SRC_KHR},
        // Blurred SSAO
        {0,
         Format::R32_SFLOAT,
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::STORE,
         RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
         RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
         ImageLayout::UNDEFINED,
         ImageLayout::COLOR_ATTACHMENT_OPTIMAL}};

    RenderPassHandle::AttachmentReference backbufferReference = {0, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    RenderPassHandle::AttachmentReference aoBlurColorRef = {1, ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    RenderPassHandle::AttachmentReference aoBlurInputRef = {1, ImageLayout::SHADER_READ_ONLY_OPTIMAL};

    RenderPassHandle::SubpassDescription aoBlurSubpass = {
        RenderPassHandle::PipelineBindPoint::GRAPHICS, {}, {aoBlurColorRef}, {}, {}, {}};

    RenderPassHandle::SubpassDescription postprocessSubpass = {
        RenderPassHandle::PipelineBindPoint::GRAPHICS, {aoBlurInputRef}, {backbufferReference}, {}, {}, {}};

    RenderPassHandle::SubpassDependency aoBlurDependency = {
        0, 1, PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT, PipelineStageFlagBits::TOP_OF_PIPE_BIT, 0, 0, 0};

    ResourceCreationContext::RenderPassCreateInfo postprocessPassInfo = {
        postprocessAttachments, {aoBlurSubpass, postprocessSubpass}, {aoBlurDependency}};
    auto postprocessRenderpass = ctx.CreateRenderPass(postprocessPassInfo);
    ResourceManager::AddResource("_Primitives/Renderpasses/postprocess.pass", postprocessRenderpass);
    return postprocessRenderpass;
}

void RenderPrimitiveFactory::CreatePrepassPipelineLayout(ResourceCreationContext & ctx)
{
    auto cameraMesh =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraMesh.layout");
    auto modelMesh =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/modelMesh.layout");

    auto prepassPipelineLayout = ctx.CreatePipelineLayout({{cameraMesh, modelMesh}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/prepass.pipelinelayout", prepassPipelineLayout);
}

void RenderPrimitiveFactory::CreateSkeletalPrepassPipelineLayout(ResourceCreationContext & ctx)
{
    auto cameraMesh =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraMesh.layout");
    auto modelMesh =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/modelMesh.layout");
    auto boneMesh =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/boneMesh.layout");

    auto prepassPipelineLayout = ctx.CreatePipelineLayout({{cameraMesh, modelMesh, boneMesh}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/prepass_skeletal.pipelinelayout", prepassPipelineLayout);
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreatePassthroughTransformPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding cameraUniformBindings[1] = {
        {0,
         DescriptorType::UNIFORM_BUFFER,
         ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT | ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
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
        {0, sizeof(VertexWithColorAndUv)}};

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
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding lightsUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto lightsMeshLayout = ctx.CreateDescriptorSetLayout({1, lightsUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/lightsMesh.layout", lightsMeshLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding cameraUniformBindings[1] = {
        {0,
         DescriptorType::UNIFORM_BUFFER,
         ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT | ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto cameraMeshLayout = ctx.CreateDescriptorSetLayout({1, cameraUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/cameraMesh.layout", cameraMeshLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding modelUniformBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto modelMeshLayout = ctx.CreateDescriptorSetLayout({1, modelUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/modelMesh.layout", modelMeshLayout);

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding materialUniformBindings[4] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {1, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {2, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {3, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto materialMeshLayout = ctx.CreateDescriptorSetLayout({4, materialUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/materialMesh.layout", materialMeshLayout);

    auto meshPipelineLayout =
        ctx.CreatePipelineLayout({{lightsMeshLayout, cameraMeshLayout, modelMeshLayout, materialMeshLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/mesh.pipelinelayout", meshPipelineLayout);
}

void RenderPrimitiveFactory::CreateMeshVertexInputState(ResourceCreationContext & ctx)
{
    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, sizeof(VertexWithNormal)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 3, false, 0},
        {0, 1, VertexComponentType::FLOAT, 3, false, 3 * sizeof(float)},
        {0, 2, VertexComponentType::FLOAT, 3, false, 6 * sizeof(float)},
        {0, 3, VertexComponentType::FLOAT, 2, false, 9 * sizeof(float)}};
    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = attributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = binding;
    auto meshInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    ResourceManager::AddResource("_Primitives/VertexInputStates/mesh.state", meshInputState);
}

void RenderPrimitiveFactory::CreateSkeletalMeshPipelineLayout(ResourceCreationContext & ctx)
{
    auto lightsMeshLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/lightsMesh.layout");
    auto cameraMeshLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraMesh.layout");

    auto modelMeshLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/modelMesh.layout");

    auto materialMeshLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/materialMesh.layout");

    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding boneBindings[1] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT}};
    auto boneLayout = ctx.CreateDescriptorSetLayout({1, boneBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/boneMesh.layout", boneLayout);

    auto skeletalMeshLayout = ctx.CreatePipelineLayout(
        {{lightsMeshLayout, cameraMeshLayout, modelMeshLayout, materialMeshLayout, boneLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/mesh_skeletal.pipelinelayout", skeletalMeshLayout);
}

void RenderPrimitiveFactory::CreateSkeletalMeshVertexInputState(ResourceCreationContext & ctx)
{
    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, sizeof(VertexWithSkinning)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 3, false, 0},
        {0, 1, VertexComponentType::FLOAT, 3, false, 3 * sizeof(float)},
        {0, 2, VertexComponentType::FLOAT, 3, false, 6 * sizeof(float)},
        {0, 3, VertexComponentType::FLOAT, 2, false, 9 * sizeof(float)},
        {0, 4, VertexComponentType::UINT, MAX_VERTEX_WEIGHTS, false, 11 * sizeof(float)},
        {0,
         5,
         VertexComponentType::FLOAT,
         MAX_VERTEX_WEIGHTS,
         false,
         11 * sizeof(float) + MAX_VERTEX_WEIGHTS * sizeof(uint32_t)},
    };
    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = attributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = binding;
    auto meshInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    ResourceManager::AddResource("_Primitives/VertexInputStates/mesh_skeletal.state", meshInputState);
}

void RenderPrimitiveFactory::CreateParticlePipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding particleEmitterUniformBindings[3] = {
        {0, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
        {1, DescriptorType::STORAGE_BUFFER, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT},
        {2, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto particleEmitterLayout = ctx.CreateDescriptorSetLayout({3, particleEmitterUniformBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/particleEmitter.layout", particleEmitterLayout);

    auto cameraMeshLayout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraMesh.layout");

    auto particleLayout = ctx.CreatePipelineLayout({{particleEmitterLayout, cameraMeshLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/particles.pipelinelayout", particleLayout);
}

void RenderPrimitiveFactory::CreateParticleVertexInputState(ResourceCreationContext & ctx)
{
    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription> binding = {
        {0, sizeof(ParticleVertex)}};

    std::vector<ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription> attributes = {
        {0, 0, VertexComponentType::FLOAT, 2, false, 0},
        {0, 1, VertexComponentType::FLOAT, 2, false, 2 * sizeof(float)},
    };
    ResourceCreationContext::VertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.vertexAttributeDescriptions = attributes;
    vertexInputStateCreateInfo.vertexBindingDescriptions = binding;
    auto particleInputState = ctx.CreateVertexInputState(vertexInputStateCreateInfo);
    ResourceManager::AddResource("_Primitives/VertexInputStates/particles.state", particleInputState);
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreateAmbientOcclusionPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding fragBindings[] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {1, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {2, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {3, DescriptorType::UNIFORM_BUFFER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
    };
    auto ambientOcclusionDSLayout = ctx.CreateDescriptorSetLayout({4, fragBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/ambientOcclusion.layout", ambientOcclusionDSLayout);

    auto ambientOcclusionLayout = ctx.CreatePipelineLayout({{ambientOcclusionDSLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/ambientOcclusion.pipelinelayout", ambientOcclusionLayout);
    return ambientOcclusionLayout;
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreateAmbientOcclusionBlurPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding fragBindings[] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto ambientOcclusionBlurDSLayout = ctx.CreateDescriptorSetLayout({1, fragBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/ambientOcclusionBlur.layout",
                                 ambientOcclusionBlurDSLayout);

    auto ambientOcclusionBlurLayout = ctx.CreatePipelineLayout({{ambientOcclusionBlurDSLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/ambientOcclusionBlur.pipelinelayout",
                                 ambientOcclusionBlurLayout);
    return ambientOcclusionBlurLayout;
}

PipelineLayoutHandle * RenderPrimitiveFactory::CreateTonemapPipelineLayout(ResourceCreationContext & ctx)
{
    ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding fragBindings[] = {
        {0, DescriptorType::COMBINED_IMAGE_SAMPLER, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT},
        {1, DescriptorType::INPUT_ATTACHMENT, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT}};
    auto tonemapDescriptorSetLayout = ctx.CreateDescriptorSetLayout({2, fragBindings});
    ResourceManager::AddResource("_Primitives/DescriptorSetLayouts/tonemap.layout", tonemapDescriptorSetLayout);

    auto tonemapLayout = ctx.CreatePipelineLayout({{tonemapDescriptorSetLayout}});
    ResourceManager::AddResource("_Primitives/PipelineLayouts/tonemap.pipelinelayout", tonemapLayout);
    return tonemapLayout;
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

void RenderPrimitiveFactory::CreatePostprocessSampler(ResourceCreationContext & ctx)
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

void RenderPrimitiveFactory::CreateDefaultNormalMap(ResourceCreationContext & ctx)
{
    ResourceCreationContext::ImageCreateInfo normalInfo;
    normalInfo.depth = 1;
    normalInfo.format = Format::RGBA8;
    normalInfo.height = 1;
    normalInfo.width = 1;
    normalInfo.mipLevels = 1;
    normalInfo.type = ImageHandle::Type::TYPE_2D;
    normalInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto normalImg = ctx.CreateImage(normalInfo);
    std::vector<uint8_t> normalData = {
        // clang-format off
        0x00, 0x00, 0x00, 0x00,
        // clang-format on
    };
    ctx.ImageData(normalImg, normalData);
    ResourceCreationContext::ImageViewCreateInfo normalViewInfo;
    normalViewInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    normalViewInfo.format = Format::RGBA8;
    normalViewInfo.image = normalImg;
    normalViewInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    normalViewInfo.subresourceRange.baseArrayLayer = 0;
    normalViewInfo.subresourceRange.baseMipLevel = 0;
    normalViewInfo.subresourceRange.layerCount = 1;
    normalViewInfo.subresourceRange.levelCount = 1;
    normalViewInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto normalView = ctx.CreateImageView(normalViewInfo);

    auto normalImage = new Image("_Primitives/Images/default_normals.img", 1, 1, false, normalImg, normalView);
    ResourceManager::AddResource("_Primitives/Images/default_normals.img", normalImage);
}

void RenderPrimitiveFactory::CreateDefaultRoughnessMap(ResourceCreationContext & ctx)
{
    ResourceCreationContext::ImageCreateInfo roughnessInfo;
    roughnessInfo.depth = 1;
    roughnessInfo.format = Format::RGBA8;
    roughnessInfo.height = 1;
    roughnessInfo.width = 1;
    roughnessInfo.mipLevels = 1;
    roughnessInfo.type = ImageHandle::Type::TYPE_2D;
    roughnessInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto roughnessImg = ctx.CreateImage(roughnessInfo);
    std::vector<uint8_t> roughnessData = {
        // clang-format off
        0xFF, 0xFF, 0xFF, 0xFF,
        // clang-format on
    };
    ctx.ImageData(roughnessImg, roughnessData);
    ResourceCreationContext::ImageViewCreateInfo roughnessViewInfo;
    roughnessViewInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    roughnessViewInfo.format = Format::RGBA8;
    roughnessViewInfo.image = roughnessImg;
    roughnessViewInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    roughnessViewInfo.subresourceRange.baseArrayLayer = 0;
    roughnessViewInfo.subresourceRange.baseMipLevel = 0;
    roughnessViewInfo.subresourceRange.layerCount = 1;
    roughnessViewInfo.subresourceRange.levelCount = 1;
    roughnessViewInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto roughnessView = ctx.CreateImageView(roughnessViewInfo);

    auto roughnessImage =
        new Image("_Primitives/Images/default_roughness.img", 1, 1, false, roughnessImg, roughnessView);
    ResourceManager::AddResource("_Primitives/Images/default_roughness.img", roughnessImage);
}

void RenderPrimitiveFactory::CreateDefaultMetallicMap(ResourceCreationContext & ctx)
{
    ResourceCreationContext::ImageCreateInfo metallicInfo;
    metallicInfo.depth = 1;
    metallicInfo.format = Format::RGBA8;
    metallicInfo.height = 1;
    metallicInfo.width = 1;
    metallicInfo.mipLevels = 1;
    metallicInfo.type = ImageHandle::Type::TYPE_2D;
    metallicInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto metallicImg = ctx.CreateImage(metallicInfo);
    std::vector<uint8_t> metallicData = {
        // clang-format off
        0x00, 0x00, 0x00, 0xFF,
        // clang-format on
    };
    ctx.ImageData(metallicImg, metallicData);
    ResourceCreationContext::ImageViewCreateInfo metallicViewInfo;
    metallicViewInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    metallicViewInfo.format = Format::RGBA8;
    metallicViewInfo.image = metallicImg;
    metallicViewInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    metallicViewInfo.subresourceRange.baseArrayLayer = 0;
    metallicViewInfo.subresourceRange.baseMipLevel = 0;
    metallicViewInfo.subresourceRange.layerCount = 1;
    metallicViewInfo.subresourceRange.levelCount = 1;
    metallicViewInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto metallicView = ctx.CreateImageView(metallicViewInfo);

    auto metallicImage = new Image("_Primitives/Images/default_metallic.img", 1, 1, false, metallicImg, metallicView);
    ResourceManager::AddResource("_Primitives/Images/default_metallic.img", metallicImage);
}

void RenderPrimitiveFactory::CreateDefaultMaterial(ResourceCreationContext & ctx)
{
    ResourceCreationContext::ImageCreateInfo defaultInfo;
    defaultInfo.depth = 1;
    defaultInfo.format = Format::RGBA8;
    defaultInfo.height = 2;
    defaultInfo.width = 2;
    defaultInfo.mipLevels = 1;
    defaultInfo.type = ImageHandle::Type::TYPE_2D;
    defaultInfo.usage =
        ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT | ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
    auto defaultImg = ctx.CreateImage(defaultInfo);
    std::vector<uint8_t> defaultData = {
        // clang-format off
        0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0xFF,
        // clang-format on
    };
    ctx.ImageData(defaultImg, defaultData);
    ResourceCreationContext::ImageViewCreateInfo defaultViewInfo;
    defaultViewInfo.components = ImageViewHandle::ComponentMapping::IDENTITY;
    defaultViewInfo.format = Format::RGBA8;
    defaultViewInfo.image = defaultImg;
    defaultViewInfo.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
    defaultViewInfo.subresourceRange.baseArrayLayer = 0;
    defaultViewInfo.subresourceRange.baseMipLevel = 0;
    defaultViewInfo.subresourceRange.layerCount = 1;
    defaultViewInfo.subresourceRange.levelCount = 1;
    defaultViewInfo.viewType = ImageViewHandle::Type::TYPE_2D;
    auto defaultView = ctx.CreateImageView(defaultViewInfo);

    auto defaultImage = new Image("_Primitives/Materials/default.mtl/image", 2, 2, false, defaultImg, defaultView);
    ResourceManager::AddResource("_Primitives/Materials/default.mtl/image", defaultImage);

    auto normals = ResourceManager::GetResource<Image>("_Primitives/Images/default_normals.img");
    auto roughness = ResourceManager::GetResource<Image>("_Primitives/Images/default_roughness.img");
    auto metallic = ResourceManager::GetResource<Image>("_Primitives/Images/default_metallic.img");

    auto defaultMaterial = new Material(defaultImage, normals, roughness, metallic);
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
                                     4 * sizeof(VertexWithColorAndUv),
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

    BufferSlice indexBufferSlice{indexBuffer, 0, 6 * sizeof(uint32_t)};
    BufferSlice vertexBufferSlice{vertexBuffer, 0, 4 * sizeof(VertexWithColorAndUv)};
    Submesh submesh("main", material, 6, indexBufferSlice, 4, vertexBufferSlice);
    auto staticMesh = new StaticMesh({submesh});
    ResourceManager::AddResource("/_Primitives/Meshes/Quad.obj", staticMesh);
}

void RenderPrimitiveFactory::CreateBoxVbo(ResourceCreationContext & ctx)
{
    std::vector<float> const boxVerts{
        // clang-format off
        // back
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.0f, 0.0f, //back bottom left
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.0f, 1.0f, //back top left
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.0f, 1.0f, //back top right
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.0f, 1.0f, //back top right
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.0f, 0.0f, //back bottom right
        -0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.0f, 0.0f, //back bottom left

		//front
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, -1.f, 0.0f, 0.0f,

		//left
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, -1.f, 0.f, 0.f, 1.0f, 0.0f,
       
        // right
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.0f, 0.0f,
        
		//bottom
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, -1.f, 0.f, 0.0f, 1.0f,

        // top
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.0f, 1.0f,
        //clang-format on
    };
    auto boxVbo = ctx.CreateBuffer({// 11 floats per vert, 6 verts per face, 6 faces
                                    sizeof(VertexWithNormal) * 6 * 6,
                                    BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                    DEVICE_LOCAL_BIT});

    ctx.BufferSubData(boxVbo, (uint8_t *)&boxVerts[0], 0, boxVerts.size() * sizeof(float));

    ResourceManager::AddResource("_Primitives/Buffers/BoxVBO.buffer", boxVbo);
}

void RenderPrimitiveFactory::CreateBoxMesh()
{
    auto vertexBuffer = ResourceManager::GetResource<BufferHandle>("_Primitives/Buffers/BoxVBO.buffer");
    auto material = ResourceManager::GetResource<Material>("_Primitives/Materials/default.mtl");

    BufferSlice vertexBufferSlice{
        vertexBuffer, 0, 6 * 6 * sizeof(VertexWithNormal)
    };
    Submesh submesh("main", material, 6 * 6, vertexBufferSlice);
    auto staticMesh = new StaticMesh({submesh});
    ResourceManager::AddResource("/_Primitives/Meshes/Box.obj", staticMesh);
}
