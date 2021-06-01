#include "SkeletalMeshLoaderAssimp.h"

#include <optional>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <optick/optick.h>

#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/BufferAllocator.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"
#include "Logging/Logger.h"
#include "SkeletalMesh.h"
#include "SkeletalMeshAnimation.h"

static auto const logger = Logger::Create("SkeletalMeshLoaderAssimp");

// If the filename looks like "x@y.fbx" and the file contains one animation, the animation will be renamed to y instead
// of whatever the name is in the file. Any other files in the same directory with the same "x" will also have their
// animations imported into this mesh.
// static std::regex const SPECIAL_ANIMATION_PATTERN("(\\w+)@(\\w+)\\.fbx$", std::regex_constants::icase);
static std::regex const SPECIAL_ANIMATION_PATTERN(".*\\W(\\w+)@(.+)\\.fbx$", std::regex_constants::icase);

std::vector<Vec3Key> ConvertV3Keys(uint32_t size, aiVectorKey * keys)
{
    std::vector<Vec3Key> ret;
    ret.reserve(size);
    for (uint32_t i = 0; i < size; ++i) {
        auto v3 = keys[i];
        ret.push_back({v3.mTime, glm::vec3(v3.mValue.x, v3.mValue.y, v3.mValue.z)});
    }
    return ret;
}

std::vector<QuatKey> ConvertQuatKeys(uint32_t size, aiQuatKey * keys)
{
    std::vector<QuatKey> ret;
    ret.reserve(size);
    for (uint32_t i = 0; i < size; ++i) {
        auto q = keys[i];
        ret.push_back({q.mTime, glm::quat(q.mValue.w, q.mValue.x, q.mValue.y, q.mValue.z)});
    }
    return ret;
}

glm::mat4 ConvertMat4(aiMatrix4x4 m4)
{
    // clang-format off
    return glm::transpose(glm::mat4(
        m4.a1, m4.a2, m4.a3, m4.a4,
        m4.b1, m4.b2, m4.b3, m4.b4,
        m4.c1, m4.c2, m4.c3, m4.c4,
        m4.d1, m4.d2, m4.d3, m4.d4
    ));
    // clang-format on
}

SkeletalMeshAnimation ConvertAnimation(aiAnimation * animation, std::optional<std::string> overrideName)
{
    std::vector<NodeAnimation> channels;
    channels.reserve(animation->mNumChannels);
    for (uint32_t j = 0; j < animation->mNumChannels; ++j) {
        auto channel = animation->mChannels[j];
        NodeAnimation converted;
        converted.nodeName = channel->mNodeName.C_Str();
        converted.positionKeys = ConvertV3Keys(channel->mNumPositionKeys, channel->mPositionKeys);
        converted.rotationKeys = ConvertQuatKeys(channel->mNumRotationKeys, channel->mRotationKeys);
        converted.scaleKeys = ConvertV3Keys(channel->mNumScalingKeys, channel->mScalingKeys);
        channels.push_back(converted);
    }
    return SkeletalMeshAnimation(
        overrideName.value_or(animation->mName.C_Str()), animation->mDuration, animation->mTicksPerSecond, channels);
}

struct NodeHierarchy {
    NodeHierarchy * parent;
    std::string self;
    glm::mat4 transform;
    std::vector<std::unique_ptr<NodeHierarchy>> children;
};

std::unique_ptr<NodeHierarchy> BuildHierarchy(aiNode * node, NodeHierarchy * parent)
{
    auto ret = std::make_unique<NodeHierarchy>();
    ret->self = node->mName.C_Str();
    ret->transform = ConvertMat4(node->mTransformation);
    ret->parent = parent;
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        ret->children.push_back(BuildHierarchy(node->mChildren[i], ret.get()));
    }
    return ret;
}

NodeHierarchy * Find(NodeHierarchy * node, std::string name)
{
    if (node->self == name) {
        return node;
    }
    for (auto const & child : node->children) {
        auto found = Find(child.get(), name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

struct BoneBuilder {
    std::string name;
    std::vector<std::vector<VertexWeight>> weights;
    glm::mat4 transform;
    glm::mat4 inverseBindMatrix;

    std::vector<uint32_t> children;
};

struct SubmeshBuilder {
    std::string name;
    std::optional<std::vector<uint32_t>> indices;
    std::vector<VertexWithSkinning> vertices;
    Material * material;
};

uint32_t CreateBoneBuilders(NodeHierarchy * node, std::vector<BoneBuilder> & out)
{
    out.push_back({});
    uint32_t idx = out.size() - 1;
    out[idx].name = node->self;
    out[idx].transform = node->transform;
    out[idx].children.reserve(node->children.size());
    for (size_t i = 0; i < node->children.size(); ++i) {
        auto childIdx = CreateBoneBuilders(node->children[i].get(), out);
        out[idx].children.push_back(childIdx);
    }
    return idx;
}

struct WeightReference {
    uint32_t vertexIdx;
    uint32_t boneIdx;
    float weight;
};

SkeletalMesh * SkeletalMeshLoaderAssimp::LoadFile(std::string const & filename)
{
    OPTICK_EVENT();
    Assimp::Importer importer;

    auto defaultNormals = ResourceManager::GetResource<Image>("_Primitives/Images/default_normals.img");
    auto defaultRoughness = ResourceManager::GetResource<Image>("_Primitives/Images/default_roughness.img");
    auto defaultMetallic = ResourceManager::GetResource<Image>("_Primitives/Images/default_metallic.img");

    std::vector<std::pair<std::filesystem::path, std::string>> additionalAnimationFiles;
    std::optional<std::string> overrideAnimName;
    std::smatch patternMatch;
    if (std::regex_match(filename, patternMatch, SPECIAL_ANIMATION_PATTERN) && patternMatch[1].matched &&
        patternMatch[2].matched) {
        std::string modelName = patternMatch[1].str();
        overrideAnimName = patternMatch[2].str();

        std::filesystem::path thisFile(filename);
        auto dir = thisFile.parent_path();
        for (auto const & sibling : std::filesystem::directory_iterator(dir)) {
            std::smatch siblingMatch;
            auto siblingPath = sibling.path();
            // Required to get regex_match template magic to understand wtf its doing??? templates were a mistake
            auto siblingString = siblingPath.string();
            if (siblingPath != thisFile && std::regex_match(siblingString, siblingMatch, SPECIAL_ANIMATION_PATTERN) &&
                siblingMatch[1].str() == patternMatch[1].str()) {
                additionalAnimationFiles.push_back(std::make_pair(siblingPath, siblingMatch[2].str()));
            }
        }
    }
    auto scene = importer.ReadFile(filename,
                                   aiProcess_LimitBoneWeights | aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                       aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    auto hierarchy = BuildHierarchy(scene->mRootNode, nullptr);

    std::vector<BoneBuilder> boneBuilders;
    CreateBoneBuilders(hierarchy.get(), boneBuilders);

    std::vector<SkeletalMeshAnimation> animations;
    animations.reserve(scene->mNumAnimations + additionalAnimationFiles.size());
    for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        animations.push_back(ConvertAnimation(
            scene->mAnimations[i],
            overrideAnimName.has_value() && scene->mNumAnimations == 1 ? overrideAnimName : std::nullopt));
    }

    for (auto const & additionalFile : additionalAnimationFiles) {
        Assimp::Importer animImporter;
        auto additionalScene =
            animImporter.ReadFile(additionalFile.first.string(),
                                  aiProcess_LimitBoneWeights | aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                      aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
        for (uint32_t i = 0; i < additionalScene->mNumAnimations; ++i) {
            animations.push_back(ConvertAnimation(additionalScene->mAnimations[i], additionalFile.second));
        }
    }

    std::unordered_map<uint32_t, Material *> materials;
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        auto material = scene->mMaterials[i];

        auto materialName = filename + '/' + material->GetName().C_Str();

        Image * albedoImage;
        for (uint32_t j = 0; j < material->mNumProperties; ++j) {
            auto prop = material->mProperties[j];
            if (strcmp(prop->mKey.C_Str(), "$clr.diffuse") == 0 && prop->mType == aiPTI_Float &&
                prop->mDataLength == 3 * sizeof(float)) {
                auto imageName = materialName + "/albedo";
                albedoImage = Image::FromData(imageName,
                                              1,
                                              1,
                                              {(uint8_t)(((float *)prop->mData)[0] * 0xFF),
                                               (uint8_t)(((float *)prop->mData)[1] * 0xFF),
                                               (uint8_t)(((float *)prop->mData)[2] * 0xFF),
                                               0xFF});
            }
        }
        // TODO: Non-default normals, roughness and metallic. But I don't have any test assets right now.
        materials.insert(
            std::make_pair(i, new Material(albedoImage, defaultNormals, defaultRoughness, defaultMetallic)));
    }

    std::vector<SubmeshBuilder> submeshBuilders;
    submeshBuilders.reserve(scene->mNumMeshes);
    size_t totalEboSize = 0;
    size_t totalVboSize = 0;
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[i];

        std::vector<WeightReference> weightRefs;
        for (uint32_t j = 0; j < mesh->mNumBones; ++j) {
            std::vector<WeightReference> weightRefsThisBone;
            auto bone = mesh->mBones[j];

            std::vector<VertexWeight> weights;
            weights.reserve(bone->mNumWeights);
            for (uint32_t k = 0; k < bone->mNumWeights; ++k) {
                auto weight = bone->mWeights[k];
                weights.push_back({weight.mVertexId, weight.mWeight});
                WeightReference ref;
                ref.vertexIdx = weight.mVertexId;
                ref.weight = weight.mWeight;
                weightRefsThisBone.push_back(ref);
            }

            glm::mat4 inverseBindMatrix = ConvertMat4(bone->mOffsetMatrix);

            auto n = Find(hierarchy.get(), bone->mName.C_Str());
            if (!n) {
                logger->Warnf("Did not find bone with name='%s' in node hierarchy", bone->mName.C_Str());
                continue;
            }
            std::optional<std::string> parent;
            if (n->parent) {
                parent = n->parent->self;
            }
            std::vector<std::string> children;
            for (auto const & child : n->children) {
                children.push_back(child->self);
            }

            for (uint32_t i = 0; i < boneBuilders.size(); ++i) {
                auto & builder = boneBuilders[i];
                if (builder.name == bone->mName.C_Str()) {
                    builder.inverseBindMatrix = inverseBindMatrix;
                    builder.weights.push_back(weights);

                    for (auto & weightRef : weightRefsThisBone) {
                        weightRef.boneIdx = i;
                    }
                    break;
                }
            }

            for (auto const & weightRef : weightRefsThisBone) {
                weightRefs.push_back(weightRef);
            }
        }

        std::vector<VertexWithSkinning> vertices;
        vertices.reserve(mesh->mNumVertices);
        for (uint32_t j = 0; j < mesh->mNumVertices; ++j) {
            auto vert = mesh->mVertices[j];
            auto norm = mesh->mNormals[j];
            VertexWithSkinning vertex;
            if (mesh->mColors[0]) {
                vertex.color = glm::vec3(mesh->mColors[0][j].r, mesh->mColors[0][j].g, mesh->mColors[0][j].b);
            } else {
                vertex.color = glm::vec3(1.f);
            }
            vertex.normal = glm::vec3(norm.x, norm.y, norm.z);
            vertex.pos = glm::vec3(vert.x, vert.y, vert.z);
            if (mesh->mTextureCoords[0]) {
                vertex.uv = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
            } else {
                vertex.uv = glm::vec2(0.f, 1.f);
            }

            for (size_t i = 0; i < MAX_VERTEX_WEIGHTS; ++i) {
                vertex.bones[i] = UINT32_MAX;
            }

            size_t weightIdx = 0;
            for (auto const & weight : weightRefs) {
                if (weight.vertexIdx == j) {
                    vertex.bones[weightIdx] = weight.boneIdx;
                    vertex.weights[weightIdx] = weight.weight;
                    ++weightIdx;
                }
            }
            vertices.push_back(vertex);
        }

        std::optional<std::vector<uint32_t>> indices;
        if (mesh->mNumFaces > 0) {
            indices = std::vector<uint32_t>();
        }
        for (uint32_t j = 0; j < mesh->mNumFaces; ++j) {
            auto face = mesh->mFaces[j];
            for (uint32_t k = 0; k < face.mNumIndices; ++k) {
                indices.value().push_back(face.mIndices[k]);
            }
        }

        Material * material;
        if (materials.find(mesh->mMaterialIndex) != materials.end()) {
            material = materials.at(mesh->mMaterialIndex);
        } else {
            material = nullptr;
        }

        if (indices.has_value()) {
            totalEboSize += indices.value().size() * sizeof(uint32_t);
        }
        totalVboSize += vertices.size() * sizeof(VertexWithSkinning);
        submeshBuilders.push_back({mesh->mName.C_Str(), indices, vertices, material});
    }

    std::vector<SkeletalBone> bones;
    for (auto const & builder : boneBuilders) {
        bones.push_back(SkeletalBone(
            builder.name, builder.weights, builder.transform, builder.inverseBindMatrix, builder.children));
    }

    std::optional<BufferSlice> indexBufferSlice;
    if (totalEboSize > 0) {
        indexBufferSlice = BufferAllocator::GetInstance()->AllocateBuffer(totalEboSize,
                                                                          BufferUsageFlags::INDEX_BUFFER_BIT |
                                                                              BufferUsageFlags::TRANSFER_DST_BIT,
                                                                          MemoryPropertyFlagBits::DEVICE_LOCAL_BIT);
    }
    auto vertexBufferSlice = BufferAllocator::GetInstance()->AllocateBuffer(totalVboSize,
                                                                            BufferUsageFlags::VERTEX_BUFFER_BIT |
                                                                                BufferUsageFlags::TRANSFER_DST_BIT,
                                                                            MemoryPropertyFlagBits::DEVICE_LOCAL_BIT);

    std::vector<Submesh> submeshes;
    submeshes.reserve(scene->mNumMeshes);
    Semaphore sem;
    ResourceManager::CreateResources(
        [&sem, &submeshBuilders, &submeshes, &indexBufferSlice, &vertexBufferSlice, totalEboSize, totalVboSize](
            ResourceCreationContext & ctx) {
            size_t eboOffset = 0;
            size_t vboOffset = 0;
            for (auto & builder : submeshBuilders) {
                std::optional<BufferSlice> submeshIndexBuffer;
                if (builder.indices.has_value() && builder.indices.value().size() > 0) {
                    size_t totalOffset = indexBufferSlice.value().GetOffset() + eboOffset;
                    size_t totalSize = builder.indices.value().size() * sizeof(uint32_t);
                    ctx.BufferSubData(indexBufferSlice.value().GetBuffer(),
                                      (uint8_t *)builder.indices.value().data(),
                                      totalOffset,
                                      totalSize);
                    eboOffset += totalSize;
                    submeshIndexBuffer = BufferSlice(indexBufferSlice.value().GetBuffer(), totalOffset, totalSize);
                }
                size_t totalOffset = vertexBufferSlice.GetOffset() + vboOffset;
                size_t totalSize = builder.vertices.size() * sizeof(VertexWithSkinning);
                ctx.BufferSubData(
                    vertexBufferSlice.GetBuffer(), (uint8_t *)builder.vertices.data(), totalOffset, totalSize);
                vboOffset += totalSize;
                BufferSlice submeshVertexBuffer(vertexBufferSlice.GetBuffer(), totalOffset, totalSize);

                if (submeshIndexBuffer.has_value()) {
                    submeshes.push_back(Submesh(builder.name,
                                                builder.material,
                                                builder.indices.value().size(),
                                                submeshIndexBuffer.value(),
                                                builder.vertices.size(),
                                                submeshVertexBuffer));
                } else {
                    submeshes.push_back(
                        Submesh(builder.name, builder.material, builder.vertices.size(), submeshVertexBuffer));
                }
            }
            sem.Signal();
        });
    sem.Wait();

    auto ret = new SkeletalMesh(
        filename, glm::inverse(ConvertMat4(scene->mRootNode->mTransformation)), bones, submeshes, animations);
    ResourceManager::AddResource(filename, ret);
    return ret;
}
