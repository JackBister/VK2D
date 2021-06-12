#include "SpriteComponent.h"

#include <glm/gtc/type_ptr.hpp>
#include <optick/optick.h>

#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/Image.h"
#include "Core/entity.h"
#include "Logging/Logger.h"
#include "Serialization/DeserializationContext.h"

static const auto logger = Logger::Create("SpriteComponent");

REFLECT_STRUCT_BEGIN(SpriteComponent)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const SPRITE_COMPONENT_SCHEMA =
    SerializedObjectSchema("SpriteComponent",
                           {
                               SerializedPropertySchema("file", SerializedValueType::STRING, {}, "", true, {},
                                                        SerializedPropertyFlags({IsFilePathFlag()})),
                               SerializedPropertySchema("isActive", SerializedValueType::BOOL, {}, "", false),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class SpriteComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return SPRITE_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto file = obj.GetString("file").value();
        auto path = ctx->workingDirectory / file;

        auto img = Image::FromFile(path.string());
        if (img == nullptr) {
            return nullptr;
        }

        auto spriteInstanceId = RenderSystem::GetInstance()->CreateSpriteInstance(img);
        bool isActive = obj.GetBool("isActive").value_or(true);
        return new SpriteComponent(spriteInstanceId, file, isActive, img);
    }
};

COMPONENT_IMPL(SpriteComponent, new SpriteComponentDeserializer())

SpriteComponent::SpriteComponent(SpriteInstanceId spriteInstanceId, std::string const & file, bool isActive,
                                 Image * image)
    : spriteInstanceId(spriteInstanceId), file(file), isActive(isActive)
{
    receiveTicks = false;
    type = "SpriteComponent";

#if HOT_RELOAD_RESOURCES
    this->image = image;
    hotReloadSubscriptionId = image->SubscribeToChanges([this](auto img) {
        this->image = img;
        this->refreshImageNextFrame = true;
    });
#endif
}

SpriteComponent::~SpriteComponent()
{
    RenderSystem::GetInstance()->DestroySpriteInstance(spriteInstanceId);

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
        Image * newImage = nullptr;
        if (refreshImageNextFrame) {
            newImage = this->image;
            refreshImageNextFrame = false;
        }
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        builder->WithSpriteInstanceUpdate({spriteInstanceId, e->GetTransform()->GetLocalToWorld(), isActive, newImage});
    }
}
