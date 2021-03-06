#pragma once

#include <string>
#include <vector>

#include "SkeletalMeshAnimation.h"
#include "Submesh.h"
#include "VertexWeight.h"

// TODO: Should NodeAnimation be "compiled" to use pointers or offsets instead of name?
// OR compile an unordered_map<string, SkeletalBone*> in SkeletalMesh constructor

class Material;

class SkeletalBone
{
public:
    SkeletalBone(std::string name, std::vector<std::vector<VertexWeight>> weights, glm::mat4 transform,
                 glm::mat4 inverseBindMatrix, std::vector<uint32_t> children)
        : name(name), weights(weights), transform(transform), inverseBindMatrix(inverseBindMatrix), children(children)
    {
    }

    inline std::string GetName() const { return name; }
    inline std::vector<std::vector<VertexWeight>> const & GetWeights() const { return weights; }
    inline glm::mat4 const & GetTransform() const { return transform; }
    inline glm::mat4 const & GetInverseBindMatrix() const { return inverseBindMatrix; }
    inline std::vector<uint32_t> const & GetChildren() const { return children; }

private:
    std::string name;
    // Weights split by submesh, so idx=0 contains weights for submesh with idx=0
    std::vector<std::vector<VertexWeight>> weights;
    glm::mat4 transform;
    // Transform of the mesh relative to the bone in bind pose
    glm::mat4 inverseBindMatrix;

    std::vector<uint32_t> children;
};

class SkeletalMesh
{
public:
    SkeletalMesh(std::string name, glm::mat4 const & inverseGlobalTransform, std::vector<SkeletalBone> bones,
                 std::vector<Submesh> submeshes, std::vector<SkeletalMeshAnimation> animations)
        : name(name), inverseGlobalTransform(inverseGlobalTransform), bones(bones), submeshes(submeshes),
          animations(animations)
    {
    }

    inline std::string GetName() const { return name; }
    inline glm::mat4 GetInverseGlobalTransform() const { return inverseGlobalTransform; }
    inline std::vector<SkeletalBone> const & GetBones() const { return bones; }
    inline std::vector<Submesh> const & GetSubmeshes() const { return submeshes; }
    inline std::vector<SkeletalMeshAnimation> const & GetAnimations() const { return animations; }

    SkeletalMeshAnimation const * GetAnimation(std::string name) const;

private:
    std::string name;
    glm::mat4 inverseGlobalTransform;
    std::vector<SkeletalBone> bones;
    std::vector<Submesh> submeshes;
    std::vector<SkeletalMeshAnimation> animations;
};
