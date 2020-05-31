#include "SkeletalMeshLoaderAssimp.h"

#include <optional>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Core/Logging/Logger.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/ResourceManager.h"
#include "SkeletalMesh.h"
#include "SkeletalMeshAnimation.h"
#include "SkeletalModel.h"

static auto const logger = Logger::Create("SkeletalMeshLoaderAssimp");

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
        ret.push_back({q.mTime, glm::quat(q.mValue.x, q.mValue.y, q.mValue.z, q.mValue.w)});
    }
    return ret;
}

glm::mat4 ConvertMat4(aiMatrix4x4 m4)
{
    // clang-format off
    return glm::mat4(
        m4.a1, m4.a2, m4.a3, m4.a4,
        m4.b1, m4.b2, m4.b3, m4.b4,
        m4.c1, m4.c2, m4.c3, m4.c4,
        m4.d1, m4.d2, m4.d3, m4.d4
    );
    // clang-format on
}

struct NodeHierarchy {
    NodeHierarchy * parent;
    std::string self;
    glm::mat4 transform;
    std::vector<std::unique_ptr<NodeHierarchy>> children;
};

NodeHierarchy * BuildHierarchy(aiNode * node, NodeHierarchy * parent)
{
    auto ret = new NodeHierarchy();
    ret->self = node->mName.C_Str();
    ret->transform = ConvertMat4(node->mTransformation);
    ret->parent = parent;
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        ret->children.push_back(std::unique_ptr<NodeHierarchy>(BuildHierarchy(node->mChildren[i], ret)));
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

void CreateRelations(NodeHierarchy * node, std::vector<BoneRelation> & out)
{
    BoneRelation relation;
    if (node->parent) {
        relation.parent = node->parent->self;
    }
    relation.self = node->self;
    for (auto const & child : node->children) {
        relation.children.push_back(child->self);
        CreateRelations(child.get(), out);
    }
    out.push_back(relation);
}

SkeletalMesh * SkeletalMeshLoaderAssimp::LoadFile(std::string const & filename)
{
    Assimp::Importer importer;
    auto scene = importer.ReadFile(filename,
                                   aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                       aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    auto hierarchy = BuildHierarchy(scene->mRootNode, nullptr);

    std::vector<SkeletalMeshAnimation> animations;
    animations.reserve(scene->mNumAnimations);
    for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        auto animation = scene->mAnimations[i];
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
        animations.emplace_back(animation->mName.C_Str(), animation->mDuration, animation->mTicksPerSecond, channels);
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
        materials.insert(std::make_pair(i, new Material(albedoImage)));
    }

    std::vector<SkeletalSubmesh> submeshes;
    submeshes.reserve(scene->mNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[i];

        std::vector<SkeletalBone> bones;
        bones.reserve(mesh->mNumBones);
        for (uint32_t j = 0; j < mesh->mNumBones; ++j) {
            auto bone = mesh->mBones[j];

            std::vector<VertexWeight> weights;
            weights.reserve(bone->mNumWeights);
            for (uint32_t k = 0; k < bone->mNumWeights; ++k) {
                auto weight = bone->mWeights[k];
                weights.push_back({weight.mVertexId, weight.mWeight});
            }

            glm::mat4 inverseBindMatrix = ConvertMat4(bone->mOffsetMatrix);

            auto n = Find(hierarchy, bone->mName.C_Str());
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
            // TODO:
            bones.push_back(
                SkeletalBone(bone->mName.C_Str(), weights, n->transform, inverseBindMatrix, parent, children));
        }

        std::vector<VertexWithNormal> vertices;
        vertices.reserve(mesh->mNumVertices);
        for (uint32_t j = 0; j < mesh->mNumVertices; ++j) {
            auto vert = mesh->mVertices[j];
            auto norm = mesh->mNormals[j];
            // auto col = mesh->mColors[j];
            // auto uv = mesh->mTextureCoords[j];
            VertexWithNormal vertex;
            /*
            if (col) {
                vertex.color = glm::vec3(col->r, col->g, col->b);
            } else {
                */
            vertex.color = glm::vec3(1.f);
            //}
            vertex.normal = glm::vec3(norm.x, norm.y, norm.z);
            vertex.pos = glm::vec3(vert.x, vert.y, vert.z);
            // TODO: wtf?
            /*
            if (uv) {
                vertex.uv = glm::vec2(uv->x, uv->y);
            } else {
            */
            vertex.uv = glm::vec2(0.f, 1.f);
            // }
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

        submeshes.push_back(SkeletalSubmesh(mesh->mName.C_Str(), bones, vertices, indices, material));
    }

    std::vector<BoneRelation> relations;
    CreateRelations(hierarchy, relations);

    auto model = new SkeletalModel();
    AssimpConverter::Convert(scene, *model);

    auto ret = new SkeletalMesh(
        filename, ConvertMat4(scene->mRootNode->mTransformation), relations, submeshes, animations, model);
    ResourceManager::AddResource(filename, ret);
    return ret;
}
