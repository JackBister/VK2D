#pragma once

#include "Component.h"
#include "Core/Rendering/SpriteInstance.h"
#include "Util/DllExport.h"

class Image;

class EAPI SpriteComponent final : public Component
{
public:
    SpriteComponent(SpriteInstanceId spriteInstanceId, std::string const & file, bool isActive, Image * image);

    ~SpriteComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    SpriteInstanceId spriteInstanceId;

    std::string file;
    bool isActive;

#if HOT_RELOAD_RESOURCES
    Image * image;
    int hotReloadSubscriptionId;
    bool refreshImageNextFrame = false;
#endif
};
