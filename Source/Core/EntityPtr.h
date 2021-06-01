#pragma once

#include <string>

#include "Util/DllExport.h  "

class Entity;
class EntityManager;

class EAPI EntityPtr
{
public:
    friend class std::hash<EntityPtr>;
    friend class EntityManager;

    EntityPtr() : entityManager(nullptr), index(0), generation(0) {}

    EntityPtr(EntityManager * entityManager, size_t index, size_t generation)
        : entityManager(entityManager), index(index), generation(generation)
    {
    }

    auto operator<=>(EntityPtr const & rhs) const = default;
    operator bool() const { return Get() != nullptr; }

    Entity * Get() const;
    EntityManager * GetManager() const { return entityManager; }

    std::string ToString() const;

private:
    EntityManager * entityManager;
    size_t index;
    size_t generation;
};

namespace std
{
template <>
struct hash<EntityPtr> {
    size_t operator()(EntityPtr const & p) const
    {
        return std::hash<void *>()((void *)p.entityManager) ^ std::hash<size_t>()(p.index) ^
               std::hash<size_t>()(p.generation);
    }
};
}
