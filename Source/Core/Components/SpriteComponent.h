#pragma once

#include <atomic>

#include "Core/Components/Component.h"
#include "Core/Rendering/SpriteInstance.h"

class Image;
class SpriteComponentDeserializer;

class SpriteComponent final : public Component
{
public:
    friend class SpriteComponentDeserializer;

    SpriteComponent()
    {
        receiveTicks = true;
        type = "SpriteComponent";
    }
    ~SpriteComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    SpriteInstanceId spriteInstance;

    std::string file;

#if HOT_RELOAD_RESOURCES
    Image * image;
    int hotReloadSubscriptionId;
#endif
};
