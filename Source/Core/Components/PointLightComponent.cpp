#include "PointLightComponent.h"

#include <optick/optick.h>

#include "Core/Rendering/RenderSystem.h"
#include "Core/entity.h"

REFLECT_STRUCT_BEGIN(PointLightComponent)
REFLECT_STRUCT_MEMBER(color)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema const POINT_LIGHT_COMPONENT_SCHEMA =
    SerializedObjectSchema("PointLightComponent",
                           {
                               SerializedPropertySchema("color", SerializedValueType::OBJECT, {}, "Vec3", true),
                               SerializedPropertySchema("isActive", SerializedValueType::BOOL),

                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class PointLightComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return POINT_LIGHT_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto colorObj = obj.GetObject("color").value();

        return new PointLightComponent(glm::vec3(
            colorObj.GetNumber("x").value(), colorObj.GetNumber("y").value(), colorObj.GetNumber("z").value()));
    }
};

COMPONENT_IMPL(PointLightComponent, new PointLightComponentDeserializer());

PointLightComponent::PointLightComponent(glm::vec3 color, bool isActive) : isActive(isActive), color(color)
{
    receiveTicks = false;
    type = "PointLightComponent";

    instanceId = RenderSystem::GetInstance()->CreatePointLightInstance(isActive, color);
}

PointLightComponent::~PointLightComponent()
{
    RenderSystem::GetInstance()->DestroyLightInstance(instanceId);
}

SerializedObject PointLightComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithBool("isActive", isActive)
        .WithObject("color",
                    SerializedObject::Builder()
                        .WithNumber("x", color.x)
                        .WithNumber("y", color.y)
                        .WithNumber("z", color.z)
                        .Build())
        .Build();
}

void PointLightComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif
    if (name == "PreRender") {
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        builder->WithLightUpdate({instanceId, isActive, e->GetTransform()->GetLocalToWorld(), color});
    }
}
