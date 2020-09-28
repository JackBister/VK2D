#pragma once

#include <functional>
#include <string>

class StaticMesh;

class StaticMeshLoader
{
public:
    virtual void LoadFile(std::string const & filename, std::function<void(StaticMesh *)> callback) = 0;
};
