#pragma once
#include <Core/Components/Component.h>
#include <Core/Reflect.h>

class BallComponentDeserializer;

class BallComponent : public Component
{
public:
    friend class BallComponentDeserializer;
    BallComponent() { receiveTicks = true; };

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    glm::vec2 velocityDir;
    float moveSpeed;
};
