#pragma once

#include <string>

class StaticMesh;

class StaticMeshLoader
{
public:
    virtual StaticMesh * LoadFile(std::string const & filename) = 0;
};
