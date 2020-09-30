#pragma once

#include <optional>
#include <variant>

#include <glm/glm.hpp>

#include "Core/Components/component.h"
#include "Core/Rendering/CameraInstance.h"

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
    CameraComponent(std::variant<OrthoCamera, PerspectiveCamera> cameraData, bool isActive = true,
                    bool defaultsToMain = false);
    ~CameraComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    inline bool IsActive() const { return isActive; }
    inline void SetActive(bool active) { this->isActive = active; }

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
    bool isActive = true;
    bool isProjectionDirty = true;
    bool isViewDirty = true;
    glm::mat4 projection;
    glm::mat4 view;

    CameraInstanceId cameraHandle;
};
