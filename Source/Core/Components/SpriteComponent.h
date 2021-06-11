#pragma once

#include <atomic>

#include "Core/Components/Component.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Util/DllExport.h"

class Image;
class SpriteComponentDeserializer;

class EAPI SpriteComponent final : public Component
{
public:
    friend class SpriteComponentDeserializer;

    SpriteComponent()
    {
        receiveTicks = false;
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
    bool isActive;

#if HOT_RELOAD_RESOURCES
    Image * image;
    int hotReloadSubscriptionId;
    bool refreshImageNextFrame = false;
#endif
};
