#pragma once

#include <string>
#include <vector>

#include "Core/EntityId.h"
#include "Core/eventarg.h"
#include "Core/transform.h"
#include "Serialization/Deserializable.h"
#include "Util/DllExport.h"
#include "Util/HashedString.h"

class Component;
class EntityManager;

class EAPI Entity final : public Deserializable
{
public:
    friend class EntityManager;

    static std::optional<Entity> Deserialize(DeserializationContext * deserializationContext,
                                             SerializedObject const & obj);

    Entity(std::string const & id, std::string const & name, Transform transform, std::vector<Component *> components)
        : id(id), name(name), transform(transform), components(components)
    {
        type = "Entity";
    }

    SerializedObject Serialize() const override;
    void FireEvent(HashedString name, EventArgs args = {});

    void AddComponent(Component * component);
    Component * GetComponent(std::string const & type) const;
    bool HasComponent(std::string const & type) const;

    inline EntityId GetId() const { return id; }
    inline std::string GetName() const { return name; }
    inline Transform * GetTransform() { return &transform; }

    REFLECT();

private:
    EntityManager * entityManager;

    EntityId id;
    std::string name;
    Transform transform;
    std::vector<Component *> components;
};
