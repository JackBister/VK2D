#include "StaticMeshComponent.h"

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/SubmittedMesh.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("StaticMeshComponent");

COMPONENT_IMPL(StaticMeshComponent, &StaticMeshComponent::s_Deserialize);

REFLECT_STRUCT_BEGIN(StaticMeshComponent)
REFLECT_STRUCT_END()

Deserializable * StaticMeshComponent::s_Deserialize(DeserializationContext * deserializationContext,
                                                    SerializedObject const & obj)
{
    auto file = obj.GetString("file").value();
    auto path = deserializationContext->workingDirectory / file;
    auto existingMesh = ResourceManager::GetResource<StaticMesh>(path.string());
    if (!existingMesh) {
        // TODO:
        logger->Errorf("existingMesh %s not found", path.string().c_str());
    }
    auto ret = new StaticMeshComponent(file, existingMesh);
    ResourceManager::CreateResources([ret](ResourceCreationContext & ctx) {
        auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
            "_Primitives/DescriptorSetLayouts/modelMesh.layout");
        ret->uniforms = ctx.CreateBuffer({sizeof(glm::mat4),
                                          BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                          DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
            ret->uniforms, 0, sizeof(glm::mat4)};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};

        ret->descriptorSet = ctx.CreateDescriptorSet({1, descriptors, layout});

        ret->hasCreatedLocalResources = true;
    });
    return ret;
}

StaticMeshComponent::StaticMeshComponent(std::string file, StaticMesh * mesh) : file(file), mesh(mesh) {}

SerializedObject StaticMeshComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", this->Reflection.name).WithString("file", this->file).Build();
}

void StaticMeshComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "Tick" && hasCreatedLocalResources) {
        auto submeshes = mesh->GetSubmeshes();
        SubmittedMesh submit;
        submit.localToWorld = entity->transform.GetLocalToWorld();
        submit.descriptorSet = descriptorSet;
        submit.uniforms = uniforms;
        for (size_t i = 0; i < submeshes.size(); ++i) {
            SubmittedSubmesh submittedSubmesh;
            auto materialDescriptor = submeshes[i].GetMaterial()->GetDescriptorSet();
            if (!materialDescriptor) {
                continue;
            }
            submittedSubmesh.materialDescriptor = materialDescriptor;
            submittedSubmesh.numIndexes = submeshes[i].GetNumIndexes();
            submittedSubmesh.indexBuffer = submeshes[i].GetIndexBuffer();
            submittedSubmesh.numVertices = submeshes[i].GetNumVertices();
            submittedSubmesh.vertexBuffer = submeshes[i].GetVertexBuffer();
            submit.submeshes.push_back(submittedSubmesh);
        }
        if (submit.submeshes.size() > 0) {
            GameModule::SubmitMesh(submit);
        }
    }
}
