#include "Core/entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Components/component.h"
#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("Entity");

DESERIALIZABLE_IMPL(Entity, &Entity::s_Deserialize)

REFLECT_STRUCT_BEGIN(Entity)
REFLECT_STRUCT_MEMBER(transform)
REFLECT_STRUCT_MEMBER(components)
REFLECT_STRUCT_END()

void Entity::FireEvent(HashedString ename, EventArgs args)
{
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

Component * Entity::GetComponent(std::string type) const
{
    for (auto const & c : components) {
        if (c->type == type) {
            return c.get();
        }
    }
    return nullptr;
}

Deserializable * Entity::s_Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj)
{
    Entity * const ret = new Entity();
    ret->name = obj.GetString("name").value();
    ret->transform = Transform::Deserialize(obj.GetObject("transform").value());
    auto serializedComponents = obj.GetArray("components");
    for (auto const & component : serializedComponents.value()) {
        if (component.index() != SerializedValue::OBJECT) {
            logger->Errorf("Unexpected non-object in component array when deserializing entity '%s'", ret->name);
        }
        Component * const c = static_cast<Component *>(
            Deserializable::Deserialize(deserializationContext, std::get<SerializedObject>(component)));
        c->entity = ret;
        ret->components.emplace_back(std::move(c));
    }
    return ret;
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
