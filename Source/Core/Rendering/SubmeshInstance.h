#pragma once

#include "Core/Rendering/StaticMeshInstance.h"
#include "Core/Resources/Material.h"
#include "Core/Resources/StaticMesh.h"

struct SubmeshInstance {
    Submesh const * submesh;
    StaticMeshInstanceId instanceId;
};

struct SubmeshInstanceComparer {
    bool operator()(SubmeshInstance const & lhs, SubmeshInstance const & rhs) const
    {
        if (lhs.submesh->GetMaterial()->GetDescriptorSet() < rhs.submesh->GetMaterial()->GetDescriptorSet()) {
            return true;
        }
        if (lhs.submesh->GetMaterial()->GetDescriptorSet() > rhs.submesh->GetMaterial()->GetDescriptorSet()) {
            return false;
        }
        // Same material
        if (lhs.submesh->GetVertexBuffer().GetBuffer() < rhs.submesh->GetVertexBuffer().GetBuffer()) {
            return true;
        }
        if (lhs.submesh->GetVertexBuffer().GetBuffer() > rhs.submesh->GetVertexBuffer().GetBuffer()) {
            return false;
        }
        // Same material, same vertex buffer
        if (!lhs.submesh->GetIndexBuffer().has_value() && !rhs.submesh->GetIndexBuffer().has_value()) {
            // Same material, same vertex buffer, no index buffers
            if (lhs.submesh->GetVertexBuffer().GetOffset() < rhs.submesh->GetVertexBuffer().GetOffset()) {
                return true;
            }
            if (lhs.submesh->GetVertexBuffer().GetOffset() > rhs.submesh->GetVertexBuffer().GetOffset()) {
                return false;
            }
            if (lhs.instanceId < rhs.instanceId) {
                return true;
            }
            if (lhs.instanceId > rhs.instanceId) {
                return false;
            }
            // Same material, same vertex buffer, no index buffers, same offset, same id
            return lhs.submesh->GetNumVertices() < rhs.submesh->GetNumVertices();
        }
        if (lhs.submesh->GetIndexBuffer().has_value() && !rhs.submesh->GetIndexBuffer().has_value()) {
            return true;
        }
        if (!lhs.submesh->GetIndexBuffer().has_value() && rhs.submesh->GetIndexBuffer().has_value()) {
            return false;
        }
        // Same material, same vertex buffer, both have index buffer
        if (lhs.submesh->GetIndexBuffer().value().GetBuffer() < rhs.submesh->GetIndexBuffer().value().GetBuffer()) {
            return true;
        }
        if (lhs.submesh->GetIndexBuffer().value().GetBuffer() > rhs.submesh->GetIndexBuffer().value().GetBuffer()) {
            return false;
        }
        // Same material, same vertex buffer, same index buffer
        if (lhs.submesh->GetIndexBuffer().value().GetOffset() < rhs.submesh->GetIndexBuffer().value().GetOffset()) {
            return true;
        }
        if (lhs.submesh->GetIndexBuffer().value().GetOffset() > rhs.submesh->GetIndexBuffer().value().GetOffset()) {
            return false;
        }
        if (lhs.instanceId < rhs.instanceId) {
            return true;
        }
        if (lhs.instanceId > rhs.instanceId) {
            return false;
        }
        // Same material, same vertex buffer, same index buffer, same index offset, same id
        return lhs.submesh->GetNumIndexes() < rhs.submesh->GetNumIndexes();
    }
};