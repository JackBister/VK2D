#pragma once

#include <vector>

#include "Submesh.h"

class StaticMesh
{
public:
    StaticMesh(std::vector<Submesh> const & submeshes) : submeshes(submeshes) {}

    inline std::vector<Submesh> const & GetSubmeshes() const { return submeshes; }

private:
    std::vector<Submesh> submeshes;
};
