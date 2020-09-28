#pragma once

#include <optional>

#include "Component.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/StaticMesh.h"

class StaticMeshComponent : public Component
{
public:
    StaticMeshComponent(std::string file, StaticMesh * mesh, bool isActive = true);
    ~StaticMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    void SetMesh(StaticMesh * mesh);

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    StaticMesh * mesh;
    bool isActive;

    std::optional<StaticMeshInstanceId> staticMeshInstance;
};
