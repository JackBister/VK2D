#pragma once

#include "SkeletalMeshLoader.h"

class SkeletalMeshLoaderAssimp : public SkeletalMeshLoader
{
public:
    SkeletalMesh * LoadFile(std::string const & filename) override;
};
