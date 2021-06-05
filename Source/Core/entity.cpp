#include "Core/entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <optick/optick.h>

#include "Core/Components/component.h"
#include "Core/EntityManager.h"
#include "Logging/Logger.h"
#include "Serialization/SchemaValidator.h"

static const auto logger = Logger::Create("Entity");

REFLECT_STRUCT_BEGIN(Entity)
REFLECT_STRUCT_MEMBER(id)
REFLECT_STRUCT_MEMBER(name)
REFLECT_STRUCT_MEMBER(transform)
REFLECT_STRUCT_MEMBER(components)
REFLECT_STRUCT_END()

static SerializedObjectSchema const ENTITY_SCHEMA = SerializedObjectSchema(
    "Entity",
    {SerializedPropertySchema::Required("id", SerializedValueType::STRING),
     SerializedPropertySchema::Required("name", SerializedValueType::STRING),
     SerializedPropertySchema::RequiredObject("transform", "Transform"),
     SerializedPropertySchema::RequiredObjectArray("components", "", SerializedPropertyFlags({IsComponentFlag()}))});

class EntityDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() { return ENTITY_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        auto entity = Entity::Deserialize(ctx, obj);
        if (!entity.has_value()) {
            return nullptr;
        }
        return new Entity(entity.value());
    }
};

DESERIALIZABLE_IMPL(Entity, new EntityDeserializer())

std::optional<Entity> Entity::Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj)
{
    auto validationResult = SchemaValidator::Validate(ENTITY_SCHEMA, obj);
    if (!validationResult.isValid) {
        logger.Warn("Failed to deserialize Entity from SerializedObject, validation failed. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger.Warn("\t{}: {}", err.first, err.second);
        }
        return std::nullopt;
    }
    auto id = obj.GetString("id").value();
    auto name = obj.GetString("name").value();
    auto transform = Transform::Deserialize(obj.GetObject("transform").value());
    auto serializedComponents = obj.GetArray("components");
    std::vector<Component *> components;
    for (auto const & component : serializedComponents.value()) {
        Component * c = static_cast<Component *>(
            Deserializable::Deserialize(deserializationContext, std::get<SerializedObject>(component)));
        if (!c) {
            logger.Error(
                "Failed to deserialize component for entity with name={}, id={}. See earlier errors.", name, id);
            for (auto c2 : components) {
                delete c2;
            }
            return std::nullopt;
        }
        components.emplace_back(c);
    }

    return Entity(id, name, transform, components);
}

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

void Entity::AddComponent(Component * component)
{
    // TODO: This is really weird
    auto ptr = entityManager->GetEntityById(id);
    component->entity = ptr;
    components.push_back(component);
    component->OnEvent("BeginPlay");
}

Component * Entity::GetComponent(std::string const & type) const
{
    for (auto const & c : components) {
        if (c->type == type) {
            return c;
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
        .WithString("id", std::string(id.ToString()))
        .WithString("name", name)
        .WithObject("transform", transform.Serialize());

    std::vector<SerializedValue> serializedComponents;
    for (auto const & c : components) {
        serializedComponents.push_back(c->Serialize());
    }

    builder.WithArray("components", serializedComponents);
    return builder.Build();
}
