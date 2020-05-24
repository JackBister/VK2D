#include "StaticMeshComponent.h"

#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/StaticMeshLoaderObj.h"
#include "Core/Serialization/DeserializationContext.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("StaticMeshComponent");

REFLECT_STRUCT_BEGIN(StaticMeshComponent)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const STATIC_MESH_COMPONENT_SCHEMA = SerializedObjectSchema(
    "StaticMeshComponent", {
                               SerializedPropertySchema("file", SerializedValueType::STRING, {}, "", true),
                               SerializedPropertySchema("isActive", SerializedValueType::BOOL),
                           });

class StaticMeshComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return STATIC_MESH_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto file = obj.GetString("file").value();
        auto path = ctx->workingDirectory / file;
        auto mesh = ResourceManager::GetResource<StaticMesh>(path.string());
        if (!mesh) {
            mesh = StaticMeshLoaderObj().LoadFile(path.string());
            if (!mesh) {
                logger->Errorf("Failed to load file='%s' when deserializing StaticMeshComponent", file.c_str());
                return nullptr;
            }
        }
        auto isActive = obj.GetBool("isActive").value_or(true);
        return new StaticMeshComponent(file, mesh, isActive);
    }
};

COMPONENT_IMPL(StaticMeshComponent, new StaticMeshComponentDeserializer());

StaticMeshComponent::~StaticMeshComponent()
{
    RenderSystem::GetInstance()->DestroyStaticMeshInstance(staticMeshInstance);
}

StaticMeshComponent::StaticMeshComponent(std::string file, StaticMesh * mesh, bool isActive)
    : file(file), mesh(mesh), isActive(isActive)
{
    staticMeshInstance = RenderSystem::GetInstance()->CreateStaticMeshInstance(mesh);

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

    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithStaticMeshInstanceUpdate(
            {staticMeshInstance, entity->GetTransform()->GetLocalToWorld(), isActive});
    }
}
