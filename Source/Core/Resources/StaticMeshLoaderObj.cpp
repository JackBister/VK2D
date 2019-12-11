#include "StaticMeshLoaderObj.h"

#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMesh.h"
#include "Core/Semaphore.h"

static const auto logger = Logger::Create("StaticMeshLoaderObj");

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;
};

StaticMesh * StaticMeshLoaderObj::LoadFile(std::string const & filename)
{
    auto baseDir = std::filesystem::path(filename).parent_path();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool loadResult =
        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), baseDir.string().c_str());

    if (!warn.empty()) {
        logger->Warnf("Warning when loading OBJ file '%s', warning='%s'", filename.c_str(), warn.c_str());
    }

    if (!err.empty()) {
        logger->Errorf("Error when loading OBJ file '%s', error='%s'", filename.c_str(), err.c_str());
    }

    if (!loadResult) {
        // TODO: return... null?
    }

    std::unordered_map<int, Material *> materialIdToMaterial;
    for (int i = 0; i < materials.size(); ++i) {
        auto & material = materials[i];

        if (!material.diffuse_texname.empty()) {
            auto albedoFile = baseDir / material.diffuse_texname;
            auto albedoImage = Image::FromFile(albedoFile.string());

            materialIdToMaterial[i] = new Material(albedoImage);
        }
    }

    std::vector<Submesh> submeshes;
    for (size_t s = 0; s < shapes.size(); ++s) {
        auto & shape = shapes[s];
        std::vector<Vertex> vertices;
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            auto numFaceVerts = shape.mesh.num_face_vertices[f];
            for (size_t v = 0; v < numFaceVerts; ++v) {
                auto idx = shape.mesh.indices[indexOffset + v];
                auto vx = attrib.vertices[3 * idx.vertex_index + 0];
                auto vy = attrib.vertices[3 * idx.vertex_index + 1];
                auto vz = attrib.vertices[3 * idx.vertex_index + 2];

                auto tx = idx.texcoord_index > -1 ? attrib.texcoords[2 * idx.texcoord_index + 0] : 0.f;
                auto ty = idx.texcoord_index > -1 ? attrib.texcoords[2 * idx.texcoord_index + 1] : 0.f;

                vertices.push_back({glm::vec3(vx, vy, vz), glm::vec3(1.f), glm::vec2(tx, ty)});
            }
            indexOffset += numFaceVerts;
        }

        auto mtlId = shape.mesh.material_ids.size() > 0 ? shape.mesh.material_ids[0] : 0;
        for (size_t i = 0; i < shape.mesh.material_ids.size(); ++i) {
            auto currentMaterialId = shape.mesh.material_ids[i];
            if (currentMaterialId != mtlId) {
                logger->Warnf("Problem loading OBJ file '%s', the engine currently requires all faces in a shape to "
                              "use the same material, but shape '%s' contains both material ID %d and %d",
                              filename.c_str(),
                              shape.name,
                              mtlId,
                              currentMaterialId);
            }
        }

        std::vector<size_t> indices(shape.mesh.indices.size());
        for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
            indices[i] = shape.mesh.indices[i].vertex_index;
        }

        auto name = shape.name;
        auto numVtx = vertices.size();
        Material * material;
        if (materialIdToMaterial.find(mtlId) == materialIdToMaterial.end()) {
            logger->Warnf("Problem loading OBJ file '%s', could not find definition for material ID %d in shape '%s'",
                          filename.c_str(),
                          mtlId,
                          shape.name.c_str());
            material = ResourceManager::GetResource<Material>("_Primitives/Materials/default.mtl");
        } else {
            material = materialIdToMaterial.at(mtlId);
        }
        BufferHandle * vbo;
        Semaphore sem;
        ResourceManager::CreateResources([&sem, &vertices, &vbo, numVtx](ResourceCreationContext & ctx) {
            vbo = ctx.CreateBuffer({numVtx * sizeof(Vertex),
                                    BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                    DEVICE_LOCAL_BIT});

            ctx.BufferSubData(vbo, (uint8_t *)&vertices[0], 0, vertices.size() * sizeof(Vertex));
            sem.Signal();
        });
        sem.Wait();

        submeshes.emplace_back(name, material, numVtx, vbo);
    }

    return new StaticMesh(submeshes);
}
