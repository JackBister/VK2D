#pragma once

#include "StaticMeshLoader.h"

class StaticMeshLoaderObj : public StaticMeshLoader
{
public:
    StaticMesh * LoadFile(std::string const & filename) override;
};
