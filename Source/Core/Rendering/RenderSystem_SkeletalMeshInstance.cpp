#include "RenderSystem.h"

#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
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
        totalVertexSize += submesh.GetVertices().size() * sizeof(VertexWithSkinning);

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
        instance.mappedVertexBuffer = (VertexWithSkinning *)ctx.MapBuffer(instance.vertexBuffer.value().GetBuffer(),
                                                                          instance.vertexBuffer.value().GetOffset(),
                                                                          instance.vertexBuffer.value().GetSize());
        if (instance.indexBuffer.has_value()) {
            instance.mappedIndexBuffer = (uint32_t *)ctx.MapBuffer(instance.indexBuffer.value().GetBuffer(),
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
               submesh.GetVertices().size() * sizeof(VertexWithSkinning));
        BufferSlice vertexBufferSlice(instance.vertexBuffer.value().GetBuffer(),
                                      instance.vertexBuffer.value().GetOffset() +
                                          currentVertexOffset * sizeof(VertexWithSkinning),
                                      submesh.GetVertices().size() * sizeof(VertexWithSkinning));
        std::optional<BufferSlice> indexBufferSlice;
        if (instance.indexBuffer.has_value()) {
            memcpy(instance.mappedIndexBuffer + currentIndexOffset,
                   submesh.GetIndices().value().data(),
                   submesh.GetIndices().value().size() * sizeof(uint32_t));
            indexBufferSlice =
                BufferSlice(instance.indexBuffer.value().GetBuffer(),
                            instance.indexBuffer.value().GetOffset() + currentIndexOffset * sizeof(uint32_t),
                            submesh.GetIndices().value().size() * sizeof(uint32_t));
            currentIndexOffset += submesh.GetIndices().value().size();
        }
        currentVertexOffset += submesh.GetVertices().size();
        submeshes.emplace_back(SkeletalSubmeshInstance(&submesh, vertexBufferSlice, indexBufferSlice));
    }

    skeletalMeshes[id].submeshes = submeshes;

    for (auto const & bone : mesh->GetBones()) {
        SkeletalBoneInstance instance;
        instance.bone = &bone;
        instance.currentTransform = bone.GetTransform();
        skeletalMeshes[id].bones.push_back(instance);
    }

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
            instance->currentAnimationName = mesh.switchToAnimation;
            instance->currentAnimation = instance->mesh->GetAnimation(mesh.switchToAnimation.value());
        }
    }
}

NodeAnimation const * FindNodeAnimation(SkeletalMeshAnimation const * anim, std::string node)
{
    for (auto const & chan : anim->GetChannels()) {
        if (chan.nodeName == node) {
            return &chan;
        }
    }
    return nullptr;
}

template <typename _Ty>
unsigned int NodeAnimation_FindIndex2(const _Ty & keys, float animationTime)
{
    for (unsigned int i = 0; i < keys.size() - 1; ++i) {
        if (animationTime < keys[i + 1].time)
            return i;
    }
    return -1;
}

glm::vec3 NodeAnimation_FindInterpolatedPosition(NodeAnimation const * nodeAnimation, float animationTime)
{
    if (nodeAnimation->positionKeys.size() == 1) {
        return nodeAnimation->positionKeys[0].value;
    }

    unsigned int positionIndex = NodeAnimation_FindIndex2(nodeAnimation->positionKeys, animationTime);
    unsigned int nextPositionIndex = (positionIndex + 1);
    float deltaTime =
        nodeAnimation->positionKeys[nextPositionIndex].time - nodeAnimation->positionKeys[positionIndex].time;
    float factor = (animationTime - nodeAnimation->positionKeys[positionIndex].time) / deltaTime;
    glm::vec3 const & startPosition = nodeAnimation->positionKeys[positionIndex].value;
    glm::vec3 const & endPosition = nodeAnimation->positionKeys[nextPositionIndex].value;

    return glm::lerp(startPosition, endPosition, factor);
}

glm::quat NodeAnimation_FindInterpolatedRotation(NodeAnimation const * nodeAnimation, float animationTime)
{
    if (nodeAnimation->rotationKeys.size() == 1) {
        return nodeAnimation->rotationKeys[0].value;
    }

    unsigned int positionIndex = NodeAnimation_FindIndex2(nodeAnimation->rotationKeys, animationTime);
    unsigned int nextPositionIndex = (positionIndex + 1);
    float deltaTime =
        nodeAnimation->rotationKeys[nextPositionIndex].time - nodeAnimation->rotationKeys[positionIndex].time;
    float factor = (animationTime - nodeAnimation->rotationKeys[positionIndex].time) / deltaTime;
    glm::quat const & startPosition = nodeAnimation->rotationKeys[positionIndex].value;
    glm::quat const & endPosition = nodeAnimation->rotationKeys[nextPositionIndex].value;

    return glm::slerp(startPosition, endPosition, factor);
}

void RenderSystem::ReadNodeHierarchy(float animationTime, SkeletalMeshAnimation const * animation,
                                     SkeletalMeshInstance * skeleton, SkeletalBoneInstance * bone,
                                     const glm::mat4x4 & parentTransform)
{
    OPTICK_EVENT();
    auto nodeName = bone->bone->GetName();
    auto nodeTransform = bone->bone->GetTransform();
    auto newNodeAnim = FindNodeAnimation(animation, nodeName);

    if (newNodeAnim) {
        glm::vec3 translation = NodeAnimation_FindInterpolatedPosition(newNodeAnim, animationTime);
        glm::quat rotation = NodeAnimation_FindInterpolatedRotation(newNodeAnim, animationTime);
        auto rotationM4 = glm::mat4_cast(rotation);
        auto translationM4 = glm::translate(glm::mat4(1.f), translation);
        nodeTransform = translationM4 * rotationM4; // * ScalingM2;
    }

    glm::mat4x4 globalTransformation = parentTransform * nodeTransform;

    bone->currentTransform =
        skeleton->mesh->GetInverseGlobalTransform() * globalTransformation * bone->bone->GetInverseBindMatrix();

    for (auto const & child : bone->bone->GetChildren()) {
        auto const & childBone = skeleton->bones[child];
        ReadNodeHierarchy(animationTime, animation, skeleton, &skeleton->bones[child], globalTransformation);
    }
}

void RenderSystem::UpdateAnimation(SkeletalMeshInstance * instance, float dt)
{
    OPTICK_EVENT();
    instance->elapsedTime = fmodf(instance->elapsedTime + dt * instance->currentAnimation->GetTicksPerSecond(),
                                  instance->currentAnimation->GetDuration());

    ReadNodeHierarchy(instance->elapsedTime, instance->currentAnimation, instance, &instance->bones[0], glm::mat4x4(1));
}

void RenderSystem::UpdateAnimations()
{
    OPTICK_EVENT();

    // TODO:
    // float deltaTime = Time::GetDeltaTime();
    float deltaTime = Time::GetUnscaledDeltaTime();

    for (auto & mesh : skeletalMeshes) {
        if (!mesh.isActive || !mesh.currentAnimationName.has_value()) {
            continue;
        }

        UpdateAnimation(&mesh, deltaTime);
    }
}