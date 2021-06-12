#include "RenderSystem.h"

#include <ThirdParty/optick/src/optick.h>

#include "Core/Resources/StaticMesh.h"

StaticMeshInstanceId RenderSystem::CreateStaticMeshInstance(StaticMesh * mesh, bool isActive)
{
    OPTICK_EVENT();
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
    OPTICK_EVENT();
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
