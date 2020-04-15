#pragma once

class RenderSystem;
class StaticMesh;

using StaticMeshInstanceId = size_t;

class StaticMeshInstance
{
    friend class RenderSystem;

private:
    StaticMeshInstanceId id;
    StaticMesh * mesh;
};