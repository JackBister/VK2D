#pragma once

#include "Component.h"
#include "Core/Rendering/SkeletalMeshInstance.h"
#include "Core/Resources/SkeletalMesh.h"

class SkeletalMeshComponent : public Component
{
public:
    SkeletalMeshComponent(std::string file, SkeletalMesh * mesh, bool isActive = true);
    ~SkeletalMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    SkeletalMesh * mesh;
    bool isActive;

    SkeletalMeshInstanceId SkeletalMeshInstance;

    bool TESTHASSTARTEDANIM = false;
};
