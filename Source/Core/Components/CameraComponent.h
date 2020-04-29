#pragma once

#include <atomic>
#include <optional>
#include <variant>

#include <glm/glm.hpp>

#include "Core/Components/component.h"
#include "Core/Rendering/CameraInstance.h"

class CameraComponentDeserializer;

struct OrthoCamera {
    float viewSize;
    float aspect;
};

struct PerspectiveCamera {
    float aspect;
    float fov;
    float zFar;
    float zNear;
};

class CameraComponent : public Component
{
public:
    friend class CameraComponentDeserializer;

    CameraComponent() { receiveTicks = true; };
    ~CameraComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    glm::mat4 const & GetProjection();
    glm::mat4 const & GetView();

    std::optional<OrthoCamera> GetOrtho();
    void SetOrtho(OrthoCamera);

    std::optional<PerspectiveCamera> GetPerspective();
    void SetPerspective(PerspectiveCamera);

    REFLECT()
    REFLECT_INHERITANCE()
private:
    enum CameraType { ORTHO = 0, PERSPECTIVE = 1 };

    std::variant<OrthoCamera, PerspectiveCamera> cameraData;

    bool defaultsToMain = false;
    bool isProjectionDirty = true;
    bool isViewDirty = true;
    glm::mat4 projection;
    glm::mat4 view;

    CameraInstanceId cameraHandle;
};
