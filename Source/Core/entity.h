#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Components/Component.h"
#include "Core/Deserializable.h"
#include "Core/HashedString.h"
#include "Core/eventarg.h"
#include "Core/transform.h"

class Entity final : public Deserializable
{
public:
    Entity(std::string const & name, Transform transform) : name(name), transform(transform) { type = "Entity"; }

    SerializedObject Serialize() const override;
    void FireEvent(HashedString name, EventArgs args = {});

    void AddComponent(std::unique_ptr<Component> component);
    Component * GetComponent(std::string const & type) const;
    bool HasComponent(std::string const & type) const;

    inline std::string GetName() { return name; }

    inline Transform * GetTransform() { return &transform; }

    REFLECT();

private:
    std::string name;
    Transform transform;
    std::vector<std::unique_ptr<Component>> components;
};
