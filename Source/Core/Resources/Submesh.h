#pragma once

#include <optional>
#include <string>

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

    inline std::string GetName() const { return name; }
    inline Material * GetMaterial() const { return material; }
    inline size_t GetNumIndexes() const { return numIndexes; }
    inline std::optional<BufferSlice> GetIndexBuffer() const { return indexBuffer; }
    inline size_t GetNumVertices() const { return numVertices; }
    inline BufferSlice GetVertexBuffer() const { return vertexBuffer; }

private:
    std::string name;

    Material * material;

    size_t numIndexes;
    std::optional<BufferSlice> indexBuffer;
    size_t numVertices;
    BufferSlice vertexBuffer;
};
