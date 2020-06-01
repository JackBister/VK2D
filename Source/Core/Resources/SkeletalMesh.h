#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Core/Rendering/Vertex.h"
#include "SkeletalMeshAnimation.h"
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

// TODO: Assimp splits submeshes by material (nice) - BUT need to verify that this does not mean that each submesh
// contains ALL vertices for the entire model!
class SkeletalSubmesh
{
public:
    SkeletalSubmesh(std::string name, std::vector<VertexWithSkinning> vertices,
                    std::optional<std::vector<uint32_t>> indices, Material * material)
        : name(name), vertices(vertices), indices(indices), material(material)
    {
    }

    inline std::string GetName() const { return name; }
    inline std::vector<VertexWithSkinning> const & GetVertices() const { return vertices; }
    inline std::optional<std::vector<uint32_t>> const & GetIndices() const { return indices; }
    inline Material * GetMaterial() const { return material; }

private:
    std::string name;
    std::vector<VertexWithSkinning> vertices;
    std::optional<std::vector<uint32_t>> indices;
    Material * material;
};

struct BoneRelation {
    std::optional<std::string> parent;
    std::string self;
    std::vector<std::string> children;
};

struct BoneAndSubmesh {
    SkeletalBone const * bone;
    size_t const submeshIdx;
};

class SkeletalMesh
{
public:
    SkeletalMesh(std::string name, glm::mat4 const & inverseGlobalTransform, std::vector<SkeletalBone> bones,
                 std::vector<SkeletalSubmesh> submeshes, std::vector<SkeletalMeshAnimation> animations)
        : name(name), inverseGlobalTransform(inverseGlobalTransform), bones(bones), submeshes(submeshes),
          animations(animations)
    {
    }

    inline std::string GetName() const { return name; }
    inline glm::mat4 GetInverseGlobalTransform() const { return inverseGlobalTransform; }
    inline std::vector<SkeletalBone> const & GetBones() const { return bones; }
    inline std::vector<SkeletalSubmesh> const & GetSubmeshes() const { return submeshes; }
    inline std::vector<SkeletalMeshAnimation> const & GetAnimations() const { return animations; }

    SkeletalMeshAnimation const * GetAnimation(std::string name) const;

private:
    std::string name;
    glm::mat4 inverseGlobalTransform;
    std::vector<SkeletalBone> bones;
    std::vector<SkeletalSubmesh> submeshes;
    std::vector<SkeletalMeshAnimation> animations;
};
