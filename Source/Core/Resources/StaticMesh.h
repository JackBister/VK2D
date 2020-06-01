#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Core/Rendering/BufferSlice.h"
#include "Submesh.h"

class StaticMesh
{
public:
    StaticMesh(std::vector<Submesh> const & submeshes);

    inline std::vector<Submesh> const & GetSubmeshes() { return submeshes; }

private:
    std::vector<Submesh> submeshes;
};
