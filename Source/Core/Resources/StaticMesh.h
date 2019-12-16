#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Core/Rendering/BufferSlice.h"

class Material;

class Submesh
{
public:
    Submesh(std::string const & name, Material * material, size_t numVertices, BufferSlice vertexBuffer)
        : name(name), numIndexes(0), material(material), numVertices(numVertices), vertexBuffer(vertexBuffer)
    {
    }

    Submesh(std::string const & name, Material * material, size_t numIndexes, BufferSlice indexBuffer,
            size_t numVertices, BufferSlice vertexBuffer)
        : name(name), material(material), numIndexes(numIndexes), indexBuffer(indexBuffer), numVertices(numVertices),
          vertexBuffer(vertexBuffer)
    {
    }

    inline Material * GetMaterial() { return material; }
    inline size_t GetNumIndexes() { return numIndexes; }
    inline std::optional<BufferSlice> GetIndexBuffer() { return indexBuffer; }
    inline size_t GetNumVertices() { return numVertices; }
    inline BufferSlice GetVertexBuffer() { return vertexBuffer; }

private:
    std::string name;

    Material * material;

    size_t numIndexes;
    std::optional<BufferSlice> indexBuffer;
    size_t numVertices;
    BufferSlice vertexBuffer;
};

class StaticMesh
{
public:
    StaticMesh(std::vector<Submesh> const & submeshes);

    inline std::vector<Submesh> GetSubmeshes() { return submeshes; }

private:
    std::vector<Submesh> submeshes;
};
