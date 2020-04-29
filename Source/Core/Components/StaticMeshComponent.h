#pragma once

#include <atomic>

#include "Component.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/StaticMesh.h"

class DescriptorSet;
class StaticMeshComponentDeserializer;

class StaticMeshComponent : public Component
{
public:
    friend class StaticMeshComponentDeserializer;

    StaticMeshComponent() { receiveTicks = true; }
    StaticMeshComponent(std::string file, StaticMesh * mesh);
    ~StaticMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    StaticMesh * mesh;

    StaticMeshInstanceId staticMeshInstance;
};
