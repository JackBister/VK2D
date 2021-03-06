#pragma once

#include <ThirdParty/glm/glm/glm.hpp>

class RenderSystem;
class StaticMesh;

using StaticMeshInstanceId = size_t;

class StaticMeshInstance
{
    friend class RenderSystem;

private:
    StaticMeshInstanceId id;
    StaticMesh * mesh;
    bool isActive;
    glm::mat4 localToWorld;
};