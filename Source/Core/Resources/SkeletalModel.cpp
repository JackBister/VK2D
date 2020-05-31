#include "SkeletalModel.h"

#include <glm/gtx/compatibility.hpp>
#include <optick/optick.h>

#include "Core/Logging/Logger.h"

/*
The MIT License (MIT)

Copyright (c) 2015 FakeTruth

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

static auto const logger = Logger::Create("SkeletalModel");

//////////////////////////////////////////////////////////////////////////
// Based on: http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html //
//////////////////////////////////////////////////////////////////////////

SkeletalModel::SkeletalModel() : m_AnimationTime(0.0f)
{
    Clear();
}

SkeletalModel::~SkeletalModel()
{
    Clear();
}

void SkeletalModel::Clear()
{
    for (unsigned int i = 0; i < m_Meshes.size(); ++i) {
        delete[] m_Meshes[i].pVertices;
        delete[] m_Meshes[i].pNormals;
        delete[] m_Meshes[i].pTransformedVertices;
        delete[] m_Meshes[i].pTransformedNormals;
        delete[] m_Meshes[i].pIndices;
    }
    m_Meshes.clear();
    m_Skeleton.Bones.clear();

    m_Animation.NodeAnimations.clear();
    m_Animation.Duration = 0.0f;
    m_Animation.TicksPerSecond = 0.0f;

    m_GlobalInverseTransform = glm::mat4x4(1);
    m_AnimationTime = 0.0f;
}

void SkeletalModel::Update(float a_Dt)
{
    OPTICK_EVENT();
    m_AnimationTime = fmodf(m_AnimationTime + a_Dt * m_Animation.TicksPerSecond, m_Animation.Duration);
    //

    ReadNodeHierarchy(m_AnimationTime, m_Animation, m_Skeleton, m_Skeleton.Bones[0], glm::mat4x4(1));
    TransformVertices(m_Skeleton);
}

void SkeletalModel::ReadNodeHierarchy(float AnimationTime, sAnimation & a_Animation, sSkeleton & a_Skeleton,
                                      sBone & a_Bone, const glm::mat4x4 & ParentTransform)
{
    OPTICK_EVENT();
    std::string NodeName(a_Bone.Name);
    glm::mat4x4 NodeTransformation(a_Bone.NodeTransform);
    const sNodeAnimation * pNewNodeAnim = FindNodeAnim(a_Animation, NodeName);

    if (pNewNodeAnim) {
        glm::vec3 Translation = NodeAnimation_FindInterpolatedPosition(*pNewNodeAnim, AnimationTime);
        glm::quat RotationQ = NodeAnimation_FindInterpolatedRotation(*pNewNodeAnim, AnimationTime);

        // 			glm::vec3 Scaling2(1, 1, 1);
        // 			glm::mat4x4 ScalingM2 = glm::scale(Scaling2);

        auto RotationM2 = glm::mat4_cast(RotationQ);
        // Original:
        // glm::mat4x4 RotationM2 = glm::toMat4(RotationQ);

        auto TranslationM2 = glm::translate(glm::mat4(1.f), Translation);
        // Original:
        // glm::mat4x4 TranslationM2 = glm::translate(Translation);

        // Combine the above transformations
        NodeTransformation = TranslationM2 * RotationM2; // * ScalingM2;
    }

    glm::mat4x4 GlobalTransformation = ParentTransform * NodeTransformation;

    unsigned int BoneIndex = Skeleton_FindBoneIndex(a_Skeleton, NodeName);
    if (BoneIndex != -1) {
        sBone * pBone = &a_Skeleton.Bones[BoneIndex];
        pBone->FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * pBone->OffsetMatrix;
    }

    for (unsigned int i = 0; i < a_Bone.children.size(); i++) {
        ReadNodeHierarchy(
            AnimationTime, a_Animation, a_Skeleton, a_Skeleton.Bones[a_Bone.children[i]], GlobalTransformation);
    }
}

void SkeletalModel::TransformVertices(const sSkeleton & a_Skeleton)
{
    OPTICK_EVENT();
    {
        OPTICK_EVENT("Main loop")
        for (unsigned int i = 0; i < m_Meshes.size(); ++i) {
            // Reset mesh vertices and normals
            sAnimatedMesh & AnimMesh = m_Meshes[i];
            memset(AnimMesh.pTransformedVertices, 0, AnimMesh.NumVertices * sizeof(glm::vec3));
            memset(AnimMesh.pTransformedNormals, 0, AnimMesh.NumVertices * sizeof(glm::vec3));

            // TODO: Change this! The outer loop should be bones and the inner meshes?!
            for (unsigned int j = 0; j < a_Skeleton.Bones.size(); ++j) {
                const sBone & Bone = a_Skeleton.Bones[j];
                //
                glm::mat4x4 Transformation = Bone.FinalTransformation;
                glm::mat3x3 Rotation = glm::mat3x3(Transformation);
                //
                if (Bone.weights.size() == 0) {
                    continue;
                }
                for (unsigned int k = 0; k < Bone.weights[i].size(); ++k) {
                    VertexWeight Weight = Bone.weights[i][k];
                    if (Weight.GetVertexIdx() > AnimMesh.NumVertices) {
                        // TODO;:
                        continue;
                    }
                    //
                    glm::vec3 inVertex = AnimMesh.pVertices[Weight.GetVertexIdx()];
                    glm::vec3 & outVertex = AnimMesh.pTransformedVertices[Weight.GetVertexIdx()];
                    outVertex += glm::vec3((Transformation * glm::vec4(inVertex, 1)) * Weight.GetWeight());
                    //
                    glm::vec3 inNormal = AnimMesh.pNormals[Weight.GetVertexIdx()];
                    glm::vec3 & outNormal = AnimMesh.pTransformedNormals[Weight.GetVertexIdx()];
                    outNormal += (Rotation * inNormal) * Weight.GetWeight();
                }
            }
        }
    }
    {
        OPTICK_EVENT("Normalize")
        // Normalize normals
        for (unsigned int i = 0; i < m_Meshes.size(); ++i) {
            sAnimatedMesh & AnimMesh = m_Meshes[i];
            for (unsigned int i = 0; i < AnimMesh.NumVertices; ++i) {
                AnimMesh.pTransformedNormals[i] = glm::normalize(AnimMesh.pTransformedNormals[i]);
            }
        }
    }
}

const sNodeAnimation * FindNodeAnim(const sAnimation & a_Animation, const std::string & a_NodeName)
{
    for (unsigned int i = 0; i < a_Animation.NodeAnimations.size(); ++i) {
        const sNodeAnimation & NodeAnim = a_Animation.NodeAnimations[i];
        if (NodeAnim.Name == a_NodeName) {
            return &NodeAnim;
        }
    }

    return NULL;
}

glm::vec3 InterpolateValue(const glm::vec3 & a_Start, const glm::vec3 & a_End, float a_Factor)
{
    return glm::lerp(a_Start, a_End, a_Factor);
}

glm::quat InterpolateValue(const glm::quat & a_Start, const glm::quat & a_End, float a_Factor)
{
    return glm::slerp(a_Start, a_End, a_Factor);
}

unsigned int Skeleton_FindBoneIndex(const sSkeleton & a_Skeleton, const std::string & a_Name)
{
    for (unsigned int i = 0; i < a_Skeleton.Bones.size(); ++i) {
        if (a_Skeleton.Bones[i].Name == a_Name)
            return i;
    }
    return -1;
}

sBone * Skeleton_FindBone(sSkeleton & a_Skeleton, const std::string & a_Name)
{
    return &a_Skeleton.Bones[Skeleton_FindBoneIndex(a_Skeleton, a_Name)];
}
