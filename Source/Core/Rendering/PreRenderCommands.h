#pragma once

#include <optional>
#include <string>
#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>

#include "CameraInstance.h"
#include "Core/Rendering/Particles/ParticleEmitter.h"
#include "LightInstance.h"
#include "SkeletalMeshInstance.h"
#include "SpriteInstance.h"
#include "StaticMeshInstance.h"

class Image;
class RenderSystem;

struct UpdateCamera {
    CameraInstanceId cameraHandle;
    glm::vec3 pos;
    glm::mat4 view;
    glm::mat4 projection;
    bool isActive;
};

struct UpdateLight {
    LightInstanceId lightInstance;
    bool isActive;
    glm::mat4 localToWorld;
    glm::vec3 color;
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
        Builder & WithLightUpdate(UpdateLight const &);
        Builder & WithSkeletalMeshUpdate(UpdateSkeletalMeshInstance const &);
        Builder & WithSpriteInstanceUpdate(UpdateSpriteInstance const &);
        Builder & WithStaticMeshInstanceUpdate(UpdateStaticMeshInstance const &);
        Builder & WithParticleEmitterUpdate(UpdateParticleEmitter const &);
        PreRenderCommands Build();

    private:
        std::vector<UpdateCamera> cameraUpdates;
        std::vector<UpdateLight> lightUpdates;
        std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates;
        std::vector<UpdateSpriteInstance> spriteUpdates;
        std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
        std::vector<UpdateParticleEmitter> particleEmitterUpdates;
    };

private:
    PreRenderCommands(std::vector<UpdateCamera> cameraUpdates, std::vector<UpdateLight> lightUpdates,
                      std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates,
                      std::vector<UpdateSpriteInstance> spriteUpdates,
                      std::vector<UpdateStaticMeshInstance> staticMeshUpdates,
                      std::vector<UpdateParticleEmitter> particleEmitterUpdates)
        : cameraUpdates(cameraUpdates), lightUpdates(lightUpdates), skeletalMeshUpdates(skeletalMeshUpdates),
          spriteUpdates(spriteUpdates), staticMeshUpdates(staticMeshUpdates),
          particleEmitterUpdates(particleEmitterUpdates)
    {
    }

    std::vector<UpdateCamera> cameraUpdates;
    std::vector<UpdateLight> lightUpdates;
    std::vector<UpdateSkeletalMeshInstance> skeletalMeshUpdates;
    std::vector<UpdateSpriteInstance> spriteUpdates;
    std::vector<UpdateStaticMeshInstance> staticMeshUpdates;
    std::vector<UpdateParticleEmitter> particleEmitterUpdates;
};
