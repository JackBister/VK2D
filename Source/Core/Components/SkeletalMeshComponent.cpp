#include "SkeletalMeshComponent.h"

#include <optick/optick.h>

#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/SkeletalMeshLoaderAssimp.h"
#include "Core/Serialization/DeserializationContext.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("SkeletalMeshComponent");

REFLECT_STRUCT_BEGIN(SkeletalMeshComponent)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const SKELETAL_MESH_COMPONENT_SCHEMA = SerializedObjectSchema(
    "SkeletalMeshComponent", {SerializedPropertySchema("file", SerializedValueType::STRING, {}, "", true, {},
                                                       {SerializedPropertyFlag::IS_FILE_PATH}),
                              SerializedPropertySchema("isActive", SerializedValueType::BOOL),
                              SerializedPropertySchema("startingAnimation", SerializedValueType::STRING)});

class SkeletalMeshComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return SKELETAL_MESH_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto file = obj.GetString("file").value();
        auto path = ctx->workingDirectory / file;
        auto mesh = ResourceManager::GetResource<SkeletalMesh>(path.string());
        if (!mesh) {
            mesh = SkeletalMeshLoaderAssimp().LoadFile(path.string());
            if (!mesh) {
                logger->Errorf("Failed to load file='%s' when deserializing StaticMeshComponent", file.c_str());
                return nullptr;
            }
        }
        auto isActive = obj.GetBool("isActive").value_or(true);
        auto startingAnimation = obj.GetString("startingAnimation");
        return new SkeletalMeshComponent(file, mesh, isActive, startingAnimation);
    }
};

COMPONENT_IMPL(SkeletalMeshComponent, new SkeletalMeshComponentDeserializer());

SkeletalMeshComponent::~SkeletalMeshComponent()
{
    RenderSystem::GetInstance()->DestroyStaticMeshInstance(SkeletalMeshInstance);
}

SkeletalMeshComponent::SkeletalMeshComponent(std::string file, SkeletalMesh * mesh, bool isActive,
                                             std::optional<std::string> startingAnimation)
    : file(file), mesh(mesh), isActive(isActive), startingAnimation(startingAnimation),
      queuedAnimationChange(startingAnimation)
{
    SkeletalMeshInstance = RenderSystem::GetInstance()->CreateSkeletalMeshInstance(mesh);

    receiveTicks = false;
    type = "StaticMeshComponent";
}

SerializedObject SkeletalMeshComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithString("file", this->file)
        .WithBool("isActive", this->isActive)
        .Build();
}

void SkeletalMeshComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithSkeletalMeshUpdate(
            {SkeletalMeshInstance, entity->GetTransform()->GetLocalToWorld(), isActive, queuedAnimationChange});
        queuedAnimationChange = std::nullopt;
    }
}

void SkeletalMeshComponent::PlayAnimation(std::string const & newAnimation)
{
    queuedAnimationChange = newAnimation;
}
