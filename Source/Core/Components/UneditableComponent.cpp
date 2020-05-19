#include "UneditableComponent.h"

REFLECT_STRUCT_BEGIN(UneditableComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const UNEDITABLE_COMPONENT_SCHEMA = SerializedObjectSchema("UneditableComponent", {});

class UneditableComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return UNEDITABLE_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        return new UneditableComponent();
    }
};

COMPONENT_IMPL(UneditableComponent, new UneditableComponentDeserializer());

SerializedObject UneditableComponent::Serialize() const
{
    return SerializedObject::Builder().WithString("type", "UneditableComponent").Build();
}

void UneditableComponent::OnEvent(HashedString name, EventArgs args) {}
