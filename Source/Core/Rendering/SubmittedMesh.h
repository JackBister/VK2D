#pragma once

#include <glm/glm.hpp>

#include "Backend/Abstract/RenderResources.h"
#include "Core/Resources/Material.h"

struct SubmittedSubmesh {
    // DescriptorSet containing the different material descriptors and buffers
    DescriptorSet * materialDescriptor;
    size_t numIndexes;
    BufferHandle * indexBuffer;
    size_t numVertices;
    BufferHandle * vertexBuffer;
};

struct SubmittedMesh {
    glm::mat4 localToWorld;
    // Buffer containing any uniforms shared by all submeshes (such as localToWorld)
    BufferHandle * uniforms;
    // DescriptorSet which binds the uniforms Buffer
    DescriptorSet * descriptorSet;

    std::vector<SubmittedSubmesh> submeshes;
};
