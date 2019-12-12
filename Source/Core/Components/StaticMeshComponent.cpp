#include "StaticMeshComponent.h"

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/SubmittedMesh.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMeshLoaderObj.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("StaticMeshComponent");

COMPONENT_IMPL(StaticMeshComponent, &StaticMeshComponent::s_Deserialize);

REFLECT_STRUCT_BEGIN(StaticMeshComponent)
REFLECT_STRUCT_END()

StaticMeshComponent::~StaticMeshComponent()
{
    RenderSystem::GetInstance()->DestroyStaticMeshInstance(staticMeshInstance);
}

Deserializable * StaticMeshComponent::s_Deserialize(DeserializationContext * deserializationContext,
                                                    SerializedObject const & obj)
{
    auto file = obj.GetString("file").value();
    auto path = deserializationContext->workingDirectory / file;
    auto existingMesh = ResourceManager::GetResource<StaticMesh>(path.string());
    if (!existingMesh) {
        existingMesh = StaticMeshLoaderObj().LoadFile(path.string());
    }
    auto ret = new StaticMeshComponent(file, existingMesh);
    ret->staticMeshInstance = RenderSystem::GetInstance()->CreateStaticMeshInstance();
    return ret;
}

StaticMeshComponent::StaticMeshComponent(std::string file, StaticMesh * mesh) : file(file), mesh(mesh) {}

SerializedObject StaticMeshComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", this->Reflection.name).WithString("file", this->file).Build();
}

void StaticMeshComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithStaticMeshInstanceUpdate({entity->transform.GetLocalToWorld(), staticMeshInstance});
    } else if (name == "Tick") {
        auto submeshes = mesh->GetSubmeshes();
        SubmittedMesh submit;
        submit.staticMeshInstance = staticMeshInstance;
        for (size_t i = 0; i < submeshes.size(); ++i) {
            SubmittedSubmesh submittedSubmesh;
            submittedSubmesh.material = submeshes[i].GetMaterial();
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
