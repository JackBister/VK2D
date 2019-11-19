#include "Core/Components/SpriteComponent.h"

#include <cstring>

#include "glm/gtc/type_ptr.hpp"

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/SubmittedSprite.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("SpriteComponent");

COMPONENT_IMPL(SpriteComponent, &SpriteComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(SpriteComponent)
REFLECT_STRUCT_END()

SpriteComponent::~SpriteComponent()
{
    auto descriptorSet = this->descriptorSet;
    auto uniforms = this->uniforms;
    ResourceManager::DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyBuffer(uniforms);
        ctx.DestroyDescriptorSet(descriptorSet);
    });

#if HOT_RELOAD_RESOURCES
    image->Unsubscribe(hotReloadSubscriptionId);
#endif
}

Deserializable * SpriteComponent::s_Deserialize(SerializedObject const & obj)
{
    SpriteComponent * ret = new SpriteComponent();
    ret->file = obj.GetString("file").value();
    auto img = Image::FromFile(ret->file);
    auto view = img->GetDefaultView();

    ResourceManager::CreateResources([ret, img, view](ResourceCreationContext & ctx) {
        auto layout =
            ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/spritePt.layout");
        ret->uniforms = ctx.CreateBuffer({sizeof(glm::mat4),
                                          BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                          DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
            ret->uniforms, 0, sizeof(glm::mat4)};

        auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {sampler, view};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uvDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, imgDescriptor}};

        ret->descriptorSet = ctx.CreateDescriptorSet({2, descriptors, layout});

        ret->hasCreatedLocalResources = true;
    });

#if HOT_RELOAD_RESOURCES
    ret->image = img;
    ret->hotReloadSubscriptionId = img->SubscribeToChanges([ret](auto img) {
        ResourceManager::CreateResources([ret, img](ResourceCreationContext & ctx) {
            auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
                "_Primitives/DescriptorSetLayouts/spritePt.layout");
            auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

            ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
                ret->uniforms, 0, sizeof(glm::mat4)};

            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {sampler,
                                                                                               img->GetDefaultView()};

            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
                {DescriptorType::UNIFORM_BUFFER, 0, uvDescriptor},
                {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, imgDescriptor}};

            auto oldDescriptorSet = ret->descriptorSet;
            logger->Infof("Updating sprite descriptorSet");
            // TODO: memory leak?
            ret->descriptorSet = ctx.CreateDescriptorSet({2, descriptors, layout});
            logger->Infof("Updated");

            ResourceManager::DestroyResources(
                [oldDescriptorSet](ResourceCreationContext & ctx) { ctx.DestroyDescriptorSet(oldDescriptorSet); });
        });
    });
#endif

    return ret;
}

SerializedObject SpriteComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", this->Reflection.name).WithString("file", file).Build();
}

void SpriteComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "Tick") {
        if (hasCreatedLocalResources) {
            SubmittedSprite submittedSprite;
            submittedSprite.descriptorSet = descriptorSet;
            submittedSprite.localToWorld = entity->transform.GetLocalToWorld();
            submittedSprite.uniforms = uniforms;
            GameModule::SubmitSprite(submittedSprite);
        }
    }
}
