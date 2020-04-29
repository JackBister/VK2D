#include "StaticMeshComponent.h"

#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/SubmittedMesh.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMeshLoaderObj.h"
#include "Core/Serialization/DeserializationContext.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("StaticMeshComponent");

REFLECT_STRUCT_BEGIN(StaticMeshComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const STATIC_MESH_COMPONENT_SCHEMA = SerializedObjectSchema({
    SerializedPropertySchema("file", SerializedValueType::STRING, {}, nullptr, true),
});

class StaticMeshComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return STATIC_MESH_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto file = obj.GetString("file").value();
        auto path = ctx->workingDirectory / file;
        auto existingMesh = ResourceManager::GetResource<StaticMesh>(path.string());
        if (!existingMesh) {
            existingMesh = StaticMeshLoaderObj().LoadFile(path.string());
        }
        auto ret = new StaticMeshComponent(file, existingMesh);
        ret->staticMeshInstance = RenderSystem::GetInstance()->CreateStaticMeshInstance(existingMesh);
        return ret;
    }
};

COMPONENT_IMPL(StaticMeshComponent, new StaticMeshComponentDeserializer());

StaticMeshComponent::~StaticMeshComponent()
{
    RenderSystem::GetInstance()->DestroyStaticMeshInstance(staticMeshInstance);
}

StaticMeshComponent::StaticMeshComponent(std::string file, StaticMesh * mesh) : file(file), mesh(mesh) {}

SerializedObject StaticMeshComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", this->Reflection.name).WithString("file", this->file).Build();
}

void StaticMeshComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithStaticMeshInstanceUpdate({entity->transform.GetLocalToWorld(), staticMeshInstance});
    } else if (name == "Tick") {
        auto submeshes = mesh->GetSubmeshes();
        SubmittedMesh submit;
        submit.staticMeshInstance = staticMeshInstance;
        submit.localToWorld = entity->transform.GetLocalToWorld();
        for (size_t i = 0; i < submeshes.size(); ++i) {
            SubmittedSubmesh submittedSubmesh{submeshes[i].GetMaterial(),
                                              submeshes[i].GetNumIndexes(),
                                              submeshes[i].GetIndexBuffer(),
                                              submeshes[i].GetNumVertices(),
                                              submeshes[i].GetVertexBuffer()};

            submit.submeshes.push_back(submittedSubmesh);
        }
        if (submit.submeshes.size() > 0) {
            GameModule::SubmitMesh(submit);
        }
    }
}
