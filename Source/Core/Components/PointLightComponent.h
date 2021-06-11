#pragma once

#include <glm/glm.hpp>

#include "Component.h"
#include "Core/Rendering/LightInstance.h"
#include "Util/DllExport.h"

class EAPI PointLightComponent : public Component
{
public:
    PointLightComponent(glm::vec3 color, bool isActive = true);
    ~PointLightComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    bool isActive;
    glm::vec3 color;

    LightInstanceId instanceId;
};
