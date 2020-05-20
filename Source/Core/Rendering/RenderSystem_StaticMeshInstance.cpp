#include "RenderSystem.h"

#include "Core/Resources/ResourceManager.h"

StaticMeshInstanceId RenderSystem::CreateStaticMeshInstance(StaticMesh * mesh, bool isActive)
{
    staticMeshes.emplace_back();
    auto id = staticMeshes.size() - 1;
    staticMeshes[id].id = id;
    staticMeshes[id].mesh = mesh;
    staticMeshes[id].isActive = isActive;
    return id;
}

void RenderSystem::DestroyStaticMeshInstance(StaticMeshInstanceId staticMesh) {}

StaticMeshInstance * RenderSystem::GetStaticMeshInstance(StaticMeshInstanceId id)
{
    return &staticMeshes[id];
}
