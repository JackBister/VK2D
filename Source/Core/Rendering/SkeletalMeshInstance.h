#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "BufferSlice.h"
#include "Core/Resources/SkeletalModel.h"
#include "Vertex.h"

class RenderSystem;
class SkeletalSubmesh;
class SkeletalMesh;

using SkeletalMeshInstanceId = size_t;

class SkeletalSubmeshInstance
{
    friend class RenderSystem;

private:
    SkeletalSubmeshInstance(SkeletalSubmesh const * submesh, BufferSlice vertexBuffer,
                            std::optional<BufferSlice> indexBuffer)
        : submesh(submesh), vertexBuffer(vertexBuffer), indexBuffer(indexBuffer)
    {
    }

    SkeletalSubmesh const * submesh;
    BufferSlice vertexBuffer;
    std::optional<BufferSlice> indexBuffer;
};

class SkeletalMeshInstance
{
    friend class RenderSystem;

private:
    SkeletalMeshInstanceId id;
    SkeletalMesh * mesh;
    SkeletalModel * model;
    bool isActive;
    glm::mat4 localToWorld;

    std::optional<std::string> currentAnimation;
    float elapsedTime;
    size_t keyIdx;

    std::vector<SkeletalSubmeshInstance> submeshes;

    // TODO: One per frameInfo? Since multiple frames can be in flight when animating
    std::optional<BufferSlice> vertexBuffer;
    VertexWithNormal * mappedVertexBuffer;

    std::optional<BufferSlice> indexBuffer;
    uint8_t * mappedIndexBuffer;
};
