#pragma once

#include "StaticMeshLoader.h"

class StaticMeshLoaderObj : public StaticMeshLoader
{
public:
    void LoadFile(std::string const & filename, std::function<void(StaticMesh *)> callback) override;
};
