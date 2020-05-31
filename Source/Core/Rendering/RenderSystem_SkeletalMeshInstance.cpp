#include "RenderSystem.h"

#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <optick/optick.h>

#include "BufferAllocator.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/DebugDrawSystem.h"
#include "Core/Resources/SkeletalMesh.h"
#include "Core/dtime.h"

static auto const logger = Logger::Create("RenderSystem_SkeletalMesh");

SkeletalMeshInstanceId RenderSystem::CreateSkeletalMeshInstance(SkeletalMesh * mesh, bool isActive)
{
    skeletalMeshes.emplace_back();
    auto id = skeletalMeshes.size() - 1;
    auto & instance = skeletalMeshes[id];
    instance.id = id;
    instance.isActive = isActive;
    instance.mesh = mesh;

    size_t totalIndexSize = 0;
    size_t totalVertexSize = 0;
    for (auto const & submesh : mesh->GetSubmeshes()) {
        totalVertexSize += submesh.GetVertices().size() * sizeof(VertexWithNormal);

        if (submesh.GetIndices().has_value()) {
            totalIndexSize += submesh.GetIndices().value().size() * sizeof(uint32_t);
        }
    }

    auto bufferAllocator = BufferAllocator::GetInstance();

    instance.vertexBuffer = bufferAllocator->AllocateBuffer(totalVertexSize,
                                                            BufferUsageFlags::VERTEX_BUFFER_BIT,
                                                            MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
                                                                MemoryPropertyFlagBits::HOST_COHERENT_BIT);

    if (totalIndexSize > 0) {
        instance.indexBuffer = bufferAllocator->AllocateBuffer(totalIndexSize * sizeof(uint32_t),
                                                               BufferUsageFlags::INDEX_BUFFER_BIT,
                                                               MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
                                                                   MemoryPropertyFlagBits::HOST_COHERENT_BIT);
    }

    Semaphore done;
    CreateResources([&done, &instance](ResourceCreationContext & ctx) {
        instance.mappedVertexBuffer = (VertexWithNormal *)ctx.MapBuffer(instance.vertexBuffer.value().GetBuffer(),
                                                                        instance.vertexBuffer.value().GetOffset(),
                                                                        instance.vertexBuffer.value().GetSize());
        if (instance.indexBuffer.has_value()) {
            instance.mappedIndexBuffer = ctx.MapBuffer(instance.indexBuffer.value().GetBuffer(),
                                                       instance.indexBuffer.value().GetOffset(),
                                                       instance.indexBuffer.value().GetSize());
        }
        done.Signal();
    });
    done.Wait();

    std::vector<SkeletalSubmeshInstance> submeshes;
    size_t currentIndexOffset = 0;
    size_t currentVertexOffset = 0;
    for (auto & submesh : mesh->GetSubmeshes()) {
        memcpy(instance.mappedVertexBuffer + currentVertexOffset,
               submesh.GetVertices().data(),
               submesh.GetVertices().size() * sizeof(VertexWithNormal));
        BufferSlice vertexBufferSlice(instance.vertexBuffer.value().GetBuffer(),
                                      instance.vertexBuffer.value().GetOffset() +
                                          currentVertexOffset * sizeof(VertexWithNormal),
                                      submesh.GetVertices().size() * sizeof(VertexWithNormal));
        std::optional<BufferSlice> indexBufferSlice;
        if (instance.indexBuffer.has_value()) {
            memcpy(instance.mappedIndexBuffer + currentIndexOffset,
                   submesh.GetIndices().value().data(),
                   submesh.GetIndices().value().size() * sizeof(uint32_t));
            indexBufferSlice = BufferSlice(instance.indexBuffer.value().GetBuffer(),
                                           instance.indexBuffer.value().GetOffset() + currentIndexOffset,
                                           submesh.GetIndices().value().size() * sizeof(uint32_t));
            currentIndexOffset += submesh.GetIndices().value().size() * sizeof(uint32_t);
        }
        currentVertexOffset += submesh.GetVertices().size();
        submeshes.emplace_back(SkeletalSubmeshInstance(&submesh, vertexBufferSlice, indexBufferSlice));
    }

    skeletalMeshes[id].submeshes = submeshes;
    skeletalMeshes[id].model = mesh->GetModel();

    return id;
}

void RenderSystem::DestroySkeletalMeshInstance(SkeletalMeshInstanceId id)
{
    skeletalMeshes[id].isActive = false;

    // TODO:
}

SkeletalMeshInstance * RenderSystem::GetSkeletalMeshInstance(SkeletalMeshInstanceId id)
{
    return &skeletalMeshes[id];
}

void RenderSystem::PreRenderSkeletalMeshes(std::vector<UpdateSkeletalMeshInstance> const & meshes)
{
    OPTICK_EVENT();

    for (auto const & mesh : meshes) {
        auto instance = GetSkeletalMeshInstance(mesh.skeletalMeshInstance);
        instance->isActive = mesh.isActive;
        instance->localToWorld = mesh.localToWorld;

        if (mesh.switchToAnimation.has_value()) {
            instance->keyIdx = 0;
            instance->elapsedTime = 0.f;
            instance->currentAnimation = mesh.switchToAnimation;

            auto newAnimation = instance->mesh->GetAnimation(mesh.switchToAnimation.value());
        }
    }
}

void RenderSystem::UpdateAnimations()
{
    OPTICK_EVENT();

    // TODO:
    // float deltaTime = Time::GetDeltaTime();
    float deltaTime = Time::GetUnscaledDeltaTime();

    for (auto & mesh : skeletalMeshes) {
        if (!mesh.isActive || !mesh.currentAnimation.has_value()) {
            continue;
        }

        mesh.model->Update(deltaTime);

        size_t offset = 0;
        for (auto const & submesh : mesh.model->m_Meshes) {
            for (size_t i = 0; i < submesh.NumVertices; ++i) {
                mesh.mappedVertexBuffer[offset + i].pos = submesh.pTransformedVertices[i];
            }
            offset += submesh.NumVertices;
        }
    }
}
