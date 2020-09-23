#include "Material.h"

#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"

Material::Material(Image * albedo, Image * normals, Image * roughness, Image * metallic)
    : albedo(albedo), normals(normals), roughness(roughness), metallic(metallic)
{
    ResourceManager::CreateResources([this](ResourceCreationContext & ctx) {
        auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
            "_Primitives/DescriptorSetLayouts/materialMesh.layout");
        auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor albedoDescriptor;
        albedoDescriptor.imageView = this->albedo->GetDefaultView();
        albedoDescriptor.sampler = sampler;

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor normalsDescriptor;
        normalsDescriptor.imageView = this->normals->GetDefaultView();
        normalsDescriptor.sampler = sampler;

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor roughnessDescriptor;
        roughnessDescriptor.imageView = this->roughness->GetDefaultView();
        roughnessDescriptor.sampler = sampler;

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor metallicDescriptor;
        metallicDescriptor.imageView = this->metallic->GetDefaultView();
        metallicDescriptor.sampler = sampler;

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[4] = {
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 0, albedoDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, normalsDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 2, roughnessDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 3, metallicDescriptor}};

        ResourceCreationContext::DescriptorSetCreateInfo descriptorSetCreateInfo;
        descriptorSetCreateInfo.descriptorCount = 4;
        descriptorSetCreateInfo.layout = layout;
        descriptorSetCreateInfo.descriptors = descriptors;
        this->descriptorSet = ctx.CreateDescriptorSet(descriptorSetCreateInfo);
    });
}
