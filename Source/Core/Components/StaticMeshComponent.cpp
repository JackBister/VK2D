#include "StaticMeshComponent.h"

#include <ThirdParty/optick/src/optick.h>

#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMeshLoaderObj.h"
#include "Core/entity.h"
#include "Jobs/JobEngine.h"
#include "Logging/Logger.h"
#include "Serialization/DeserializationContext.h"

static const auto logger = Logger::Create("StaticMeshComponent");

REFLECT_STRUCT_BEGIN(StaticMeshComponent)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const STATIC_MESH_COMPONENT_SCHEMA =
    SerializedObjectSchema("StaticMeshComponent",
                           {
                               SerializedPropertySchema("file", SerializedValueType::STRING, {}, "", true, {},
                                                        SerializedPropertyFlags({IsFilePathFlag()})),
                               SerializedPropertySchema("isActive", SerializedValueType::BOOL),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class StaticMeshComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return STATIC_MESH_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto file = obj.GetString("file").value();
        auto path = ctx->workingDirectory / file;
        auto isActive = obj.GetBool("isActive").value_or(true);
        auto mesh = ResourceManager::GetResource<StaticMesh>(path.string());
        auto ret = new StaticMeshComponent(file, mesh, isActive);
        if (!mesh) {
            StaticMeshLoaderObj().LoadFile(path.string(), [path, ret](StaticMesh * mesh) {
                if (!mesh) {
                    logger.Error("Failed to load file='{}' when deserializing StaticMeshComponent");
                    return;
                }
                ret->SetMesh(mesh);
            });
        }
        return ret;
    }
};

COMPONENT_IMPL(StaticMeshComponent, new StaticMeshComponentDeserializer());

StaticMeshComponent::~StaticMeshComponent()
{
    // TODO: There is a potential race with the MeshLoader, if the MeshLoader loads after the StaticMeshComponent is
    // destroyed the engine will crash
    if (staticMeshInstance.has_value()) {
        RenderSystem::GetInstance()->DestroyStaticMeshInstance(staticMeshInstance.value());
    }
}

StaticMeshComponent::StaticMeshComponent(std::string file, StaticMesh * mesh, bool isActive)
    : file(file), mesh(mesh), isActive(isActive)
{
    if (mesh) {
        staticMeshInstance = RenderSystem::GetInstance()->CreateStaticMeshInstance(mesh);
    }

    receiveTicks = false;
    type = "StaticMeshComponent";
}

SerializedObject StaticMeshComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithString("file", this->file)
        .WithBool("isActive", this->isActive)
        .Build();
}

void StaticMeshComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "PreRender" && staticMeshInstance.has_value()) {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        builder->WithStaticMeshInstanceUpdate(
            {staticMeshInstance.value(), e->GetTransform()->GetLocalToWorld(), isActive});
    }
}

void StaticMeshComponent::SetMesh(StaticMesh * mesh)
{
    this->mesh = mesh;
    this->staticMeshInstance = RenderSystem::GetInstance()->CreateStaticMeshInstance(mesh, this->isActive);
}
