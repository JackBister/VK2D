#pragma once
#include <Core/Components/Component.h>

class PaddleComponent : public Component
{
public:
    static Deserializable * s_Deserialize(DeserializationContext * deserializationContext,
                                          SerializedObject const & obj);

    PaddleComponent() { receiveTicks = true; };

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
