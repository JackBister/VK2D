#pragma once

#include "Component.h"
#include "Core/Rendering/SkeletalMeshInstance.h"
#include "Util/DllExport.h"

class SkeletalMesh;

class EAPI SkeletalMeshComponent : public Component
{
public:
    SkeletalMeshComponent(std::string file, SkeletalMesh * mesh, bool isActive = true,
                          std::optional<std::string> startingAnimation = std::nullopt);
    ~SkeletalMeshComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    void PlayAnimation(std::string const & newAnimation);

    REFLECT()
    REFLECT_INHERITANCE()

private:
    std::string file;
    SkeletalMesh * mesh;
    bool isActive;
    std::optional<std::string> startingAnimation;

    std::optional<std::string> queuedAnimationChange;

    SkeletalMeshInstanceId SkeletalMeshInstance;
};
