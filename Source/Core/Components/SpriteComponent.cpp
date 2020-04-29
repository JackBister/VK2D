#include "Core/Components/SpriteComponent.h"

#include <cstring>

#include "glm/gtc/type_ptr.hpp"
#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/SubmittedSprite.h"
#include "Core/Resources/Image.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("SpriteComponent");

REFLECT_STRUCT_BEGIN(SpriteComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const SPRITE_COMPONENT_SCHEMA = SerializedObjectSchema({
    SerializedPropertySchema("file", SerializedValueType::STRING, {}, {}, true),
});

class SpriteComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return SPRITE_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        SpriteComponent * ret = new SpriteComponent();
        ret->file = obj.GetString("file").value();

        auto path = ctx->workingDirectory / ret->file;

        auto img = Image::FromFile(path.string());

        ret->spriteInstance = RenderSystem::GetInstance()->CreateSpriteInstance(img);

#if HOT_RELOAD_RESOURCES
        ret->image = img;
        ret->hotReloadSubscriptionId = img->SubscribeToChanges([ret](auto img) {
#if 0
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
#endif
        });
#endif

        return ret;
    }
};

COMPONENT_IMPL(SpriteComponent, new SpriteComponentDeserializer())

SpriteComponent::~SpriteComponent()
{
    RenderSystem::GetInstance()->DestroySpriteInstance(spriteInstance);

#if HOT_RELOAD_RESOURCES
    image->Unsubscribe(hotReloadSubscriptionId);
#endif
}

SerializedObject SpriteComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", this->Reflection.name).WithString("file", file).Build();
}

void SpriteComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithSpriteInstanceUpdate({entity->transform.GetLocalToWorld(), spriteInstance});
    } else if (name == "Tick") {
        SubmittedSprite submittedSprite;
        submittedSprite.spriteInstance = spriteInstance;
        GameModule::SubmitSprite(submittedSprite);
    }
}
