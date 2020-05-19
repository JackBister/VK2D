#pragma once
#include <Core/Components/Component.h>

class PaddleComponentDeserializer;

class PaddleComponent : public Component
{
public:
    friend class PaddleComponentDeserializer;

    PaddleComponent()
    {
        receiveTicks = true;
        type = "PaddleComponent";
    };

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    float flapSpeed = 40.f;
    float gravity = 50.f;
    bool isColliding = false;
    float velocityY = 0.f;
};
