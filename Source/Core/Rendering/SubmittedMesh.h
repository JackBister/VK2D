#pragma once
#include <optional>

#include <glm/glm.hpp>

#include "Backend/Abstract/RenderResources.h"
#include "Core/Rendering/BufferSlice.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/Material.h"

struct SubmittedSubmesh {
    Material * material;
    size_t numIndexes;
    std::optional<BufferSlice> indexBuffer;
    size_t numVertices;
    BufferSlice vertexBuffer;
};

struct SubmittedMesh {
    StaticMeshInstanceId staticMeshInstance;
    glm::mat4 localToWorld;

    std::vector<SubmittedSubmesh> submeshes;
};
