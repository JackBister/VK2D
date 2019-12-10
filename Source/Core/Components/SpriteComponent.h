#pragma once

#include <atomic>

#include "Core/Components/Component.h"
#include "Core/Rendering/SpriteInstance.h"

class Image;

class SpriteComponent final : public Component
{
public:
    static Deserializable * s_Deserialize(DeserializationContext * deserializationContext,
                                          SerializedObject const & str);

    SpriteComponent() { receiveTicks = true; }
    ~SpriteComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    SpriteInstance spriteInstance;

    std::string file;

#if HOT_RELOAD_RESOURCES
    Image * image;
    int hotReloadSubscriptionId;
#endif
};
