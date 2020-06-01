#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "BufferSlice.h"
#include "Vertex.h"

class RenderSystem;
class SkeletalMeshAnimation;
class SkeletalSubmesh;
class SkeletalBone;
class SkeletalMesh;

using SkeletalMeshInstanceId = size_t;

class SkeletalBoneInstance
{
    friend class RenderSystem;

private:
    SkeletalBone const * bone;
    glm::mat4 currentTransform;
};

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
    bool isActive;
    glm::mat4 localToWorld;

    std::optional<std::string> currentAnimationName;
    SkeletalMeshAnimation const * currentAnimation;
    float elapsedTime;
    size_t keyIdx;

    std::vector<SkeletalBoneInstance> bones;
    std::vector<SkeletalSubmeshInstance> submeshes;

    // TODO: One per frameInfo? Since multiple frames can be in flight when animating
    std::optional<BufferSlice> vertexBuffer;
    VertexWithSkinning * mappedVertexBuffer;

    std::optional<BufferSlice> indexBuffer;
    uint32_t * mappedIndexBuffer;
};
