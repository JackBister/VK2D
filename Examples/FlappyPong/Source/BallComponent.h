#pragma once
#include <Core/Components/Component.h>
#include <Core/Reflect.h>

class BallComponent : public Component
{
public:
    static Deserializable * s_Deserialize(SerializedObject const & obj);

    BallComponent() { receiveTicks = true; };

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    glm::vec2 velocityDir;
    float moveSpeed;
};
