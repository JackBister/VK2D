#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "CameraInstance.h"
#include "SpriteInstance.h"
#include "StaticMeshInstance.h"

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
