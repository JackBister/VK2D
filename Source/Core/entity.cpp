#include "Core/entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <optick/optick.h>

#include "Core/Components/component.h"
#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("Entity");

REFLECT_STRUCT_BEGIN(Entity)
REFLECT_STRUCT_MEMBER(transform)
REFLECT_STRUCT_MEMBER(components)
REFLECT_STRUCT_END()

static SerializedObjectSchema const ENTITY_SCHEMA = SerializedObjectSchema(
    "Entity",
    {
        SerializedPropertySchema("name", SerializedValueType::STRING, {}, {}, true),
        SerializedPropertySchema("transform", SerializedValueType::OBJECT, {}, "Transform", true),
        SerializedPropertySchema("components", SerializedValueType::ARRAY, SerializedValueType::OBJECT, "", true),
    });

class EntityDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() { return ENTITY_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        Entity * const ret = new Entity();
        ret->name = obj.GetString("name").value();
        ret->transform = Transform::Deserialize(obj.GetObject("transform").value());
        auto serializedComponents = obj.GetArray("components");
        for (auto const & component : serializedComponents.value()) {
            Component * const c =
                static_cast<Component *>(Deserializable::Deserialize(ctx, std::get<SerializedObject>(component)));
            if (!c) {
                logger->Errorf("Not adding component to entity=%s because deserialization failed. See earlier errors.",
                               ret->name.c_str());
                continue;
            }
            c->entity = ret;
            ret->components.emplace_back(std::move(c));
        }
        return ret;
    }
};

DESERIALIZABLE_IMPL(Entity, new EntityDeserializer())

void Entity::FireEvent(HashedString ename, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", ename.c_str());
#endif

    for (auto const & c : components) {
        if (c->isActive) {
            // Only send tick event if component::receiveTicks is true
            if (ename != "Tick" || c->receiveTicks) {
                c->OnEvent(ename, args);
            }
            if (ename == "EndPlay") {
                c->isActive = false;
            }
        }
    }
}

void Entity::AddComponent(std::unique_ptr<Component> component)
{
    component->entity = this;
    components.push_back(std::move(component));
}

Component * Entity::GetComponent(std::string const & type) const
{
    for (auto const & c : components) {
        if (c->type == type) {
            return c.get();
        }
    }
    return nullptr;
}

bool Entity::HasComponent(std::string const & type) const
{
    for (auto const & c : components) {
        if (c->type == type) {
            return true;
        }
    }
    return false;
}

SerializedObject Entity::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", this->Reflection.name)
        .WithString("name", name)
        .WithObject("transform", transform.Serialize());

    std::vector<SerializedValue> serializedComponents;
    for (auto const & c : components) {
        serializedComponents.push_back(c->Serialize());
    }

    builder.WithArray("components", serializedComponents);
    return builder.Build();
}
