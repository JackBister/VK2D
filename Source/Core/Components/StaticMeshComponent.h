#pragma once

#include <atomic>

#include "Component.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/StaticMesh.h"

class DescriptorSet;

class StaticMeshComponent : public Component
{
public:
    StaticMeshComponent(std::string file, StaticMesh * mesh, bool isActive = true);
    ~StaticMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    StaticMesh * mesh;
    bool isActive;

    StaticMeshInstanceId staticMeshInstance;
};
