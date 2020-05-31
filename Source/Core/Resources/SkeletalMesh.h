#pragma once

#include <optional>
#include <string>
#include <vector>

#include "AssimpConverter.h"
#include "Core/Rendering/Vertex.h"
#include "SkeletalMeshAnimation.h"
#include "SkeletalModel.h"
#include "VertexWeight.h"

// TODO: Should NodeAnimation be "compiled" to use pointers or offsets instead of name?
// OR compile an unordered_map<string, SkeletalBone*> in SkeletalMesh constructor

class Material;

class SkeletalBone
{
public:
    SkeletalBone(std::string name, std::vector<VertexWeight> weights, glm::mat4 transform, glm::mat4 inverseBindMatrix,
                 std::optional<std::string> parent, std::vector<std::string> children)
        : name(name), weights(weights), transform(transform), inverseBindMatrix(inverseBindMatrix), parent(parent),
          children(children)
    {
    }

    inline std::string GetName() const { return name; }
    inline std::vector<VertexWeight> const & GetWeights() const { return weights; }
    inline glm::mat4 const & GetTransform() const { return transform; }
    inline glm::mat4 const & GetInverseBindMatrix() const { return inverseBindMatrix; }
    inline std::optional<std::string> GetParent() const { return parent; }
    inline std::vector<std::string> const & GetChildren() const { return children; }

private:
    std::string name;
    std::vector<VertexWeight> weights;
    // TODO: What even is this? Taken from the node hierarchy in assimp. Is it different from inverseBindMatrix?
    glm::mat4 transform;
    // Transform of the mesh relative to the bone in bind pose
    glm::mat4 inverseBindMatrix;

    // TODO: Should be replaced with pointers
    std::optional<std::string> parent;
    std::vector<std::string> children;
};

// TODO: Assimp splits submeshes by material (nice) - BUT need to verify that this does not mean that each submesh
// contains ALL vertices for the entire model!
class SkeletalSubmesh
{
public:
    SkeletalSubmesh(std::string name, std::vector<SkeletalBone> bones, std::vector<VertexWithNormal> vertices,
                    std::optional<std::vector<uint32_t>> indices, Material * material)
        : name(name), bones(bones), vertices(vertices), indices(indices), material(material)
    {
    }

    inline std::string GetName() const { return name; }
    inline std::vector<SkeletalBone> const & GetBones() const { return bones; }
    inline std::vector<VertexWithNormal> const & GetVertices() const { return vertices; }
    inline std::optional<std::vector<uint32_t>> const & GetIndices() const { return indices; }
    inline Material * GetMaterial() const { return material; }

private:
    std::string name;
    std::vector<SkeletalBone> bones;
    std::vector<VertexWithNormal> vertices;
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
    SkeletalMesh(std::string name, glm::mat4 const & rootBoneToModel, std::vector<BoneRelation> boneRelations,
                 std::vector<SkeletalSubmesh> submeshes, std::vector<SkeletalMeshAnimation> animations,
                 SkeletalModel * model)
        : name(name), rootBoneToModel(rootBoneToModel), boneRelations(boneRelations), submeshes(submeshes),
          animations(animations), model(model)
    {
    }

    inline std::string GetName() const { return name; }
    inline glm::mat4 GetRootBoneToModel() const { return rootBoneToModel; }
    inline std::vector<BoneRelation> const & GetBoneRelations() const { return boneRelations; }
    inline std::vector<SkeletalSubmesh> const & GetSubmeshes() const { return submeshes; }
    inline std::vector<SkeletalMeshAnimation> const & GetAnimations() const { return animations; }
    SkeletalMeshAnimation const * GetAnimation(std::string const & name) const;
    std::vector<BoneAndSubmesh> GetBone(std::string const & name) const;
    std::vector<BoneAndSubmesh> GetRootBone() const;
    std::vector<BoneRelation> GetLeafBones() const;

    inline SkeletalModel * GetModel() const { return model; }

private:
    std::string name;
    glm::mat4 rootBoneToModel;
    std::vector<BoneRelation> boneRelations;
    std::vector<SkeletalSubmesh> submeshes;
    std::vector<SkeletalMeshAnimation> animations;

    // TODO:
    SkeletalModel * model;
};
