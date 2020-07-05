#pragma once

#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "CameraInstance.h"
#include "SkeletalMeshInstance.h"
#include "SpriteInstance.h"
#include "StaticMeshInstance.h"

class Image;
class RenderSystem;

struct UpdateCamera {
    CameraInstanceId cameraHandle;
    glm::mat4 view;
    glm::mat4 projection;
    bool isActive;
};

struct UpdateSpriteInstance {
    SpriteInstanceId spriteInstance;
    glm::mat4 localToWorld;
    bool isActive;

    Image * newImage;
};

struct UpdateSkeletalMeshInstance {
    SkeletalMeshInstanceId skeletalMeshInstance;
    glm::mat4 localToWorld;
    bool isActive;

    std::optional<std::string> switchToAnimation;
};

struct UpdateStaticMeshInstance {
    StaticMeshInstanceId staticMeshInstance;
    glm::mat4 localToWorld;
    bool isActive;
};

class PreRenderCommands
{
    friend class RenderSystem;

public:
    class Builder
    {
    public:
        Builder & WithCameraUpdate(UpdateCamera const &);
        Builder & WithSkeletalMeshUpdate(UpdateSkeletalMeshInstance const &);
        Builder & WithSpriteInstanceUpdate(UpdateSpriteInstance const &);
        Builder & WithStaticMeshInstanceUpdate(UpdateStaticMeshInstance const &);
        PreRenderCommands Build();

    private:
        std::vector<UpdateCamera> cameraUpdates;
        std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates;
        std::vector<UpdateSpriteInstance> spriteUpdates;
        std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
    };

private:
    PreRenderCommands(std::vector<UpdateCamera> cameraUpdates,
                      std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates,
                      std::vector<UpdateSpriteInstance> spriteUpdates,
                      std::vector<UpdateStaticMeshInstance> staticMeshUpdates)
        : cameraUpdates(cameraUpdates), skeletalMeshUpdates(skeletalMeshUpdates), spriteUpdates(spriteUpdates),
          staticMeshUpdates(staticMeshUpdates)
    {
    }

    std::vector<UpdateCamera> cameraUpdates;
    std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates;
    std::vector<UpdateSpriteInstance> spriteUpdates;
    std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
};
