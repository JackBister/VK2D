#pragma once

#include <string>

class SkeletalMesh;

class SkeletalMeshLoader
{
public:
    virtual SkeletalMesh * LoadFile(std::string const & filename) = 0;
};
