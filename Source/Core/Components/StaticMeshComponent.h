#pragma once

#include <atomic>

#include "Component.h"
#include "Core/Resources/StaticMesh.h"

class DescriptorSet;

class StaticMeshComponent : public Component
{
public:
    static Deserializable * s_Deserialize(DeserializationContext * deserializationContext,
                                          SerializedObject const & obj);

    StaticMeshComponent() { receiveTicks = true; }
    StaticMeshComponent(std::string file, StaticMesh * mesh);

    // ~StaticMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    StaticMesh * mesh;

    DescriptorSet * descriptorSet;
    BufferHandle * uniforms;

    std::atomic<bool> hasCreatedLocalResources{false};
};
