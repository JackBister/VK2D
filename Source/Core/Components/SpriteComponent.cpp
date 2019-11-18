#include "Core/Components/SpriteComponent.h"

#include <cstring>

#include "glm/gtc/type_ptr.hpp"
#include "nlohmann/json.hpp"

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/Image.h"
#include "Core/Rendering/SubmittedSprite.h"
#include "Core/entity.h"
#include "Core/scene.h"

static const auto logger = Logger::Create("SpriteComponent");

COMPONENT_IMPL(SpriteComponent, &SpriteComponent::s_Deserialize)

SpriteComponent::~SpriteComponent()
{
    GameModule::CreateResources([this](ResourceCreationContext & ctx) {
        ctx.DestroyBuffer(uniforms);
        ctx.DestroyDescriptorSet(descriptorSet);
    });

#if HOT_RELOAD_RESOURCES
    image->Unsubscribe(hotReloadSubscriptionId);
#endif
}

Deserializable * SpriteComponent::s_Deserialize(std::string const & str)
{
    SpriteComponent * ret = new SpriteComponent();
    auto j = nlohmann::json::parse(str);
    ret->file = j["file"].get<std::string>();
    auto img = Image::FromFile(j["file"]);
    // TODO: There is a deadlock here if GetDefaultView is called inside CreateResources on OpenGL
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

std::string SpriteComponent::Serialize() const
{
    nlohmann::json j;
    j["type"] = this->type;
    j["file"] = file;
    return j.dump();
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
