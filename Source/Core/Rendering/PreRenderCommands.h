#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "CameraInstance.h"
#include "SpriteInstance.h"
#include "StaticMeshInstance.h"

class RenderSystem;

struct UpdateCamera {
    glm::mat4 view;
    glm::mat4 projection;
    CameraInstanceId cameraHandle;
};

struct UpdateSpriteInstance {
    glm::mat4 localToWorld;
    SpriteInstanceId spriteInstance;
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
        Builder & WithSpriteInstanceUpdate(UpdateSpriteInstance const &);
        Builder & WithStaticMeshInstanceUpdate(UpdateStaticMeshInstance const &);
        PreRenderCommands Build();

    private:
        std::vector<UpdateCamera> cameraUpdates;
        std::vector<UpdateSpriteInstance> spriteUpdates;
        std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
    };

private:
    PreRenderCommands(std::vector<UpdateCamera> cameraUpdates, std::vector<UpdateSpriteInstance> spriteUpdates,
                      std::vector<UpdateStaticMeshInstance> staticMeshUpdates)
        : cameraUpdates(cameraUpdates), spriteUpdates(spriteUpdates), staticMeshUpdates(staticMeshUpdates)
    {
    }

    std::vector<UpdateCamera> cameraUpdates;
    std::vector<UpdateSpriteInstance> spriteUpdates;
    std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
};
