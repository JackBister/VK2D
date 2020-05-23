#include "RenderSystem.h"

#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMesh.h"

StaticMeshInstanceId RenderSystem::CreateStaticMeshInstance(StaticMesh * mesh, bool isActive)
{
    staticMeshes.emplace_back();
    auto id = staticMeshes.size() - 1;
    staticMeshes[id].id = id;
    staticMeshes[id].mesh = mesh;
    staticMeshes[id].isActive = isActive;

    for (auto const & submesh : mesh->GetSubmeshes()) {
        sortedSubmeshInstances.insert({&submesh, id});
    }
    return id;
}

void RenderSystem::DestroyStaticMeshInstance(StaticMeshInstanceId id)
{
    // TODO: Do this properly
    staticMeshes[id].isActive = false;

    for (auto i = sortedSubmeshInstances.begin(); i != sortedSubmeshInstances.end();) {
        if (i->instanceId == id) {
            sortedSubmeshInstances.erase(i++);
        } else {
            ++i;
        }
    }
}

StaticMeshInstance * RenderSystem::GetStaticMeshInstance(StaticMeshInstanceId id)
{
    return &staticMeshes[id];
}
