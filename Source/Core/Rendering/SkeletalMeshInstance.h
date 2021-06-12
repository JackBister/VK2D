#pragma once

#include <optional>

#include <glm/glm.hpp>

class RenderSystem;
class SkeletalMeshAnimation;
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

    std::vector<SkeletalBoneInstance> bones;
};
