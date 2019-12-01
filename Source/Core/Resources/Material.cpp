#include "Material.h"

#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"

Material::Material(Image * albedo) : albedo(albedo)
{
    ResourceManager::CreateResources([this](ResourceCreationContext & ctx) {
        auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
            "_Primitives/DescriptorSetLayouts/materialMesh.layout");
        auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor albedoDescriptor;
        albedoDescriptor.imageView = this->albedo->GetDefaultView();
        albedoDescriptor.sampler = sampler;

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[1] = {
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, albedoDescriptor}};

        ResourceCreationContext::DescriptorSetCreateInfo descriptorSetCreateInfo;
        descriptorSetCreateInfo.descriptorCount = 1;
        descriptorSetCreateInfo.layout = layout;
        descriptorSetCreateInfo.descriptors = descriptors;
        this->descriptorSet = ctx.CreateDescriptorSet(descriptorSetCreateInfo);
    });
}
