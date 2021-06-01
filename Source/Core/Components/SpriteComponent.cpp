﻿#include "Core/Components/SpriteComponent.h"

#include <cstring>

#include "glm/gtc/type_ptr.hpp"
#include <optick/optick.h>

#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/Image.h"
#include "Core/entity.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("SpriteComponent");

REFLECT_STRUCT_BEGIN(SpriteComponent)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const SPRITE_COMPONENT_SCHEMA =
    SerializedObjectSchema("SpriteComponent",
                           {
                               SerializedPropertySchema("file", SerializedValueType::STRING, {}, "", true, {},
                                                        {SerializedPropertyFlag::IS_FILE_PATH}),
                               SerializedPropertySchema("isActive", SerializedValueType::BOOL, {}, "", false),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

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
        ret->isActive = obj.GetBool("isActive").value_or(true);

#if HOT_RELOAD_RESOURCES
        ret->image = img;
        ret->hotReloadSubscriptionId = img->SubscribeToChanges([ret](auto img) {
            ret->image = img;
            ret->refreshImageNextFrame = true;
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
        builder->WithSpriteInstanceUpdate({spriteInstance, e->GetTransform()->GetLocalToWorld(), isActive, newImage});
    }
}
