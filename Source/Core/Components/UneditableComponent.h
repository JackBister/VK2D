#pragma once

#include "Component.h"

class UneditableComponent : public Component
{
public:
    UneditableComponent()
    {
        receiveTicks = false;
        type = "UneditableComponent";
    }

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()
};
