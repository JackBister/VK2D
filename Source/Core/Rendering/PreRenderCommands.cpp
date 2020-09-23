#include "PreRenderCommands.h"

PreRenderCommands::Builder & PreRenderCommands::Builder::WithCameraUpdate(UpdateCamera const & updateCamera)
{
    cameraUpdates.push_back(updateCamera);
    return *this;
}

PreRenderCommands::Builder & PreRenderCommands::Builder::WithLightUpdate(UpdateLight const & UpdateLight)
{
    lightUpdates.push_back(UpdateLight);
    return *this;
}

PreRenderCommands::Builder &
PreRenderCommands::Builder::WithSkeletalMeshUpdate(UpdateSkeletalMeshInstance const & updateSkeletalMesh)
{
    skeletalMeshUpdates.push_back(updateSkeletalMesh);
    return *this;
}

PreRenderCommands::Builder &
PreRenderCommands::Builder::WithSpriteInstanceUpdate(UpdateSpriteInstance const & updateSprite)
{
    spriteUpdates.push_back(updateSprite);
    return *this;
}

PreRenderCommands::Builder &
PreRenderCommands::Builder::WithStaticMeshInstanceUpdate(UpdateStaticMeshInstance const & updateStaticMesh)
{
    staticMeshUpdates.push_back(updateStaticMesh);
    return *this;
}

PreRenderCommands PreRenderCommands::Builder::Build()
{
    return PreRenderCommands(cameraUpdates, lightUpdates, skeletalMeshUpdates, spriteUpdates, staticMeshUpdates);
}
