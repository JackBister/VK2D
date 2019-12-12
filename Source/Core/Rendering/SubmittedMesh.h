#pragma once

#include <glm/glm.hpp>

#include "Backend/Abstract/RenderResources.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/Material.h"

struct SubmittedSubmesh {
    Material * material;
    size_t numIndexes;
    BufferHandle * indexBuffer;
    size_t numVertices;
    BufferHandle * vertexBuffer;
};

struct SubmittedMesh {
    StaticMeshInstance staticMeshInstance;

    std::vector<SubmittedSubmesh> submeshes;
};
