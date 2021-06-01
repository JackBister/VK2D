#include "SkeletalMesh.h"

#include "Logging/Logger.h"

static auto const logger = Logger::Create("SkeletalMesh");

SkeletalMeshAnimation const * SkeletalMesh::GetAnimation(std::string name) const
{
    for (auto const & anim : animations) {
        if (anim.GetName() == name) {
            return &anim;
        }
    }
    return nullptr;
}
