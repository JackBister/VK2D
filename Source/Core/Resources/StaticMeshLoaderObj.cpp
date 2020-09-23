#include "StaticMeshLoaderObj.h"

#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/BufferAllocator.h"
#include "Core/Rendering/Vertex.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMesh.h"
#include "Core/Semaphore.h"

static const auto logger = Logger::Create("StaticMeshLoaderObj");

struct CpuSubmesh {
    std::string name;
    Material * material;
    std::vector<VertexWithNormal> vertices;
};

StaticMesh * StaticMeshLoaderObj::LoadFile(std::string const & filename)
{
    auto baseDir = std::filesystem::path(filename).parent_path();

    auto defaultNormals = ResourceManager::GetResource<Image>("_Primitives/Images/default_normals.img");

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
        return nullptr;
    }

    if (!loadResult) {
        return nullptr;
    }

    std::unordered_map<int, Material *> materialIdToMaterial;
    for (int i = 0; i < materials.size(); ++i) {
        auto & material = materials[i];

        Image * albedoImage;
        Image * normalsImage;
        Image * roughnessImage;
        Image * metallicImage;
        if (!material.diffuse_texname.empty()) {
            auto albedoFile = baseDir / material.diffuse_texname;
            albedoImage = Image::FromFile(albedoFile.string());
        } else {
            uint8_t r = material.diffuse[0] >= 1.f ? 0xFF : material.diffuse[0] * 256;
            uint8_t g = material.diffuse[1] >= 1.f ? 0xFF : material.diffuse[1] * 256;
            uint8_t b = material.diffuse[2] >= 1.f ? 0xFF : material.diffuse[2] * 256;
            auto albedoFile = filename + '/' + material.name + "/albedo";
            albedoImage = Image::FromData(albedoFile, 1, 1, {r, g, b, 0xFF});
        }
        if (!material.normal_texname.empty()) {
            auto normalsFile = baseDir / material.normal_texname;
            normalsImage = Image::FromFile(normalsFile.string());
        } else if (!material.bump_texname.empty()) {
            auto normalsFile = baseDir / material.bump_texname;
            normalsImage = Image::FromFile(normalsFile.string());
        } else {
            normalsImage = defaultNormals;
        }
        if (!material.roughness_texname.empty()) {
            auto roughnessFile = baseDir / material.roughness_texname;
            roughnessImage = Image::FromFile(roughnessFile.string());
        } else {
            uint8_t r = material.roughness >= 1.f ? 0xFF : material.roughness * 256;
            uint8_t g = material.roughness >= 1.f ? 0xFF : material.roughness * 256;
            uint8_t b = material.roughness >= 1.f ? 0xFF : material.roughness * 256;
            auto roughnessFile = filename + '/' + material.name + "/roughness";
            roughnessImage = Image::FromData(roughnessFile, 1, 1, {r, g, b, 0xFF});
        }
        if (!material.metallic_texname.empty()) {
            auto metallicFile = baseDir / material.metallic_texname;
            metallicImage = Image::FromFile(metallicFile.string());
        } else {
            uint8_t r = material.metallic >= 1.f ? 0xFF : material.metallic * 256;
            uint8_t g = material.metallic >= 1.f ? 0xFF : material.metallic * 256;
            uint8_t b = material.metallic >= 1.f ? 0xFF : material.metallic * 256;
            auto metallicFile = filename + '/' + material.name + "/metallic";
            metallicImage = Image::FromData(metallicFile, 1, 1, {r, g, b, 0xFF});
        }
        materialIdToMaterial[i] = new Material(albedoImage, normalsImage, roughnessImage, metallicImage);
    }

    std::vector<CpuSubmesh> cpuSubmeshes;
    size_t totalVboSize = 0;
    for (size_t s = 0; s < shapes.size(); ++s) {
        auto & shape = shapes[s];
        std::vector<VertexWithNormal> vertices;
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            auto numFaceVerts = shape.mesh.num_face_vertices[f];
            for (size_t v = 0; v < numFaceVerts; ++v) {
                auto idx = shape.mesh.indices[indexOffset + v];
                auto vx = attrib.vertices[3 * idx.vertex_index + 0];
                auto vy = attrib.vertices[3 * idx.vertex_index + 1];
                auto vz = attrib.vertices[3 * idx.vertex_index + 2];

                auto nx = idx.normal_index > -1 ? attrib.normals[3 * idx.normal_index + 0] : 0.f;
                auto ny = idx.normal_index > -1 ? attrib.normals[3 * idx.normal_index + 1] : 0.f;
                auto nz = idx.normal_index > -1 ? attrib.normals[3 * idx.normal_index + 2] : 0.f;

                auto tx = idx.texcoord_index > -1 ? attrib.texcoords[2 * idx.texcoord_index + 0] : 0.f;
                auto ty = idx.texcoord_index > -1 ? attrib.texcoords[2 * idx.texcoord_index + 1] : 0.f;

                vertices.push_back({glm::vec3(vx, vy, vz), glm::vec3(1.f), glm::vec3(nx, ny, nz), glm::vec2(tx, ty)});
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

        CpuSubmesh cpuSubmesh;
        cpuSubmesh.name = name;
        cpuSubmesh.material = material;
        cpuSubmesh.vertices = vertices;
        cpuSubmeshes.push_back(cpuSubmesh);
        totalVboSize += vertices.size() * sizeof(VertexWithNormal);
    }

    auto bufferAllocator = BufferAllocator::GetInstance();
    auto buffer =
        bufferAllocator->AllocateBuffer(totalVboSize,
                                        BufferUsageFlags::TRANSFER_DST_BIT | BufferUsageFlags::VERTEX_BUFFER_BIT,
                                        MemoryPropertyFlagBits::DEVICE_LOCAL_BIT);
    std::vector<Submesh> submeshes;
    Semaphore sem;
    ResourceManager::CreateResources([&sem, &cpuSubmeshes, &submeshes, &buffer](ResourceCreationContext & ctx) {
        size_t offset = 0;
        for (auto & submesh : cpuSubmeshes) {
            size_t size = submesh.vertices.size() * sizeof(VertexWithNormal);
            size_t totalOffset = buffer.GetOffset() + offset;
            ctx.BufferSubData(buffer.GetBuffer(),
                              (uint8_t *)&submesh.vertices[0],
                              totalOffset,
                              submesh.vertices.size() * sizeof(VertexWithNormal));
            submeshes.emplace_back(submesh.name,
                                   submesh.material,
                                   submesh.vertices.size(),
                                   BufferSlice(buffer.GetBuffer(), totalOffset, size));
            offset += size;
        }
        sem.Signal();
    });
    sem.Wait();

    auto ret = new StaticMesh(submeshes);
    ResourceManager::AddResource(filename, ret);
    return ret;
}
