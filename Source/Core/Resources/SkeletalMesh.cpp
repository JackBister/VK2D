#include "SkeletalMesh.h"

#include "Core/Logging/Logger.h"

static auto const logger = Logger::Create("SkeletalMesh");

SkeletalMeshAnimation const * SkeletalMesh::GetAnimation(std::string const & name) const
{
    for (auto const & animation : animations) {
        if (animation.GetName() == name) {
            return &animation;
        }
    }
    return nullptr;
}

std::vector<BoneAndSubmesh> SkeletalMesh::GetBone(std::string const & name) const
{
    std::vector<BoneAndSubmesh> ret;
    for (size_t i = 0; i < submeshes.size(); ++i) {
        for (auto const & bone : submeshes[i].GetBones()) {
            if (bone.GetName() == name) {
                ret.push_back({&bone, i});
            }
        }
    }
    return ret;
}

std::vector<BoneAndSubmesh> SkeletalMesh::GetRootBone() const
{
    for (size_t i = 0; i < boneRelations.size(); ++i) {
        if (!boneRelations[i].parent.has_value() || boneRelations[i].parent.value() == "RootNode") {
            logger->Infof("root name='%s'", boneRelations[i].self.c_str());
        }
    }

    for (size_t i = 0; i < boneRelations.size(); ++i) {
        if (!boneRelations[i].parent.has_value() || boneRelations[i].parent.value() == "RootNode") {
            return GetBone(boneRelations[i].self);
        }
    }
    return {};
}

std::vector<BoneRelation> SkeletalMesh::GetLeafBones() const
{
    std::vector<BoneRelation> relations;
    for (size_t i = 0; i < boneRelations.size(); ++i) {
        if (boneRelations[i].children.size() == 0) {
            relations.push_back(boneRelations[i]);
        }
    }

    return relations;
}
