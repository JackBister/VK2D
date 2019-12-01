#pragma once

#include <string>
#include <vector>

class BufferHandle;
class Material;

class Submesh
{
public:
    Submesh(std::string const & name, Material * material, size_t numVertices, BufferHandle * vertexBuffer)
        : name(name), numIndexes(0), indexBuffer(nullptr), material(material), numVertices(numVertices),
          vertexBuffer(vertexBuffer)
    {
    }
    Submesh(std::string const & name, Material * material, size_t numIndexes, BufferHandle * indexBuffer,
            size_t numVertices, BufferHandle * vertexBuffer)
        : name(name), material(material), numIndexes(numIndexes), indexBuffer(indexBuffer), numVertices(numVertices),
          vertexBuffer(vertexBuffer)
    {
    }

    inline Material * GetMaterial() { return material; }
    inline size_t GetNumIndexes() { return numIndexes; }
    inline BufferHandle * GetIndexBuffer() { return indexBuffer; }
    inline size_t GetNumVertices() { return numVertices; }
    inline BufferHandle * GetVertexBuffer() { return vertexBuffer; }

private:
    std::string name;

    Material * material;

    size_t numIndexes;
    BufferHandle * indexBuffer;
    size_t numVertices;
    BufferHandle * vertexBuffer;
};

class StaticMesh
{
public:
    // static StaticMesh * FromFile(std::string const & filename);

    StaticMesh(std::vector<Submesh> const & submeshes);

    inline std::vector<Submesh> GetSubmeshes() { return submeshes; }

private:
    std::vector<Submesh> submeshes;
};
