#include "EntityManager.h"

#include "Core/Components/Component.h"
#include "Logging/Logger.h"

static auto const logger = Logger::Create("EntityManager");

std::string const EntityManager::IS_MAIN_CAMERA_TAG = "IsMainCamera";

EntityManager * EntityManager::GetInstance()
{
    static EntityManager singletonEntityManager;
    return &singletonEntityManager;
}

EntityPtr EntityManager::AddEntity(Entity e)
{
    std::scoped_lock lock(liveEntitiesLock);
    size_t index = 0;
    size_t generation = 0;
    if (freeSlots.size() > 0) {
        index = freeSlots.front().index;
        generation = freeSlots.front().generation;
        freeSlots.pop_front();
        liveEntities[index] = LiveEntity{.generation = generation, .isRemoved = false, .entity = e};
    } else {
        liveEntities.push_back(LiveEntity{.generation = 0, .isRemoved = false, .entity = e});
        index = liveEntities.size() - 1;
        generation = 0;
    }
    liveEntities[index].entity.entityManager = this;
    for (auto c : liveEntities[index].entity.components) {
        c->entity = EntityPtr(this, index, generation);
    }

    EntityPtr ret(this, index, generation);
    auto id = liveEntities[index].entity.GetId();
    idToPtr[id] = ret;
    liveEntities[index].entity.FireEvent("BeginPlay");
    return ret;
}

void EntityManager::RemoveEntity(EntityPtr ptr)
{
    std::scoped_lock lock(liveEntitiesLock);
    auto & e = liveEntities[ptr.index];
    if (e.generation != ptr.generation) {
        logger.Warn("Generation mismatch when attempting to remove an entity. ptr={}, actualGeneration={}",
                    ptr.ToString(),
                    e.generation);
        return;
    }
    if (e.isRemoved) {
        logger.Warn("Attempt to remove an already removed entity. ptr={}", ptr.ToString());
        return;
    }
    for (auto c : e.entity.components) {
        // TODO: This is not great
        delete c;
    }
    e.entity.components.clear();
    e.isRemoved = true;
    freeSlots.push_back(FreeSlot{.index = ptr.index, .generation = e.generation + 1});
}

Entity * EntityManager::Get(EntityPtr ptr) const
{
    if (!IsValid(ptr)) {
        return nullptr;
    }
    return const_cast<Entity *>(&liveEntities[ptr.index].entity);
}

EntityPtr EntityManager::First()
{
    for (size_t i = 0; i < liveEntities.size(); ++i) {
        if (!liveEntities[i].isRemoved) {
            return EntityPtr(this, i, liveEntities[i].generation);
        }
    }
    return EntityPtr();
}

EntityPtr EntityManager::Next(EntityPtr current)
{
    for (size_t i = current.index + 1; i < liveEntities.size(); ++i) {
        if (!liveEntities[i].isRemoved) {
            return EntityPtr(this, i, liveEntities[i].generation);
        }
    }
    // Wraparound
    for (size_t i = 0; i < current.index; ++i) {
        if (!liveEntities[i].isRemoved) {
            return EntityPtr(this, i, liveEntities[i].generation);
        }
    }
    // Null
    return EntityPtr();
}

EntityPtr EntityManager::Prev(EntityPtr current)
{
    if (current.index != 0) {
        for (size_t i = current.index - 1;; --i) {
            if (!liveEntities[i].isRemoved) {
                return EntityPtr(this, i, liveEntities[i].generation);
            }
            if (i == 0) {
                break;
            }
        }
    }
    // Wraparound
    for (size_t i = liveEntities.size() - 1; i > current.index; --i) {
        if (!liveEntities[i].isRemoved) {
            return EntityPtr(this, i, liveEntities[i].generation);
        }
    }
    // Null
    return EntityPtr();
}

EntityPtr EntityManager::GetEntityById(EntityId id)
{
    if (idToPtr.find(id) == idToPtr.end()) {
        return EntityPtr();
    }
    return idToPtr[id];
}

// A "singleton tag" is a tag which can only be applied to a single entity at a time.
// For example, only one entity can be the main camera.
std::optional<EntityPtr> EntityManager::GetEntityBySingletonTag(std::string tag)
{
    if (singletonTags.find(tag) == singletonTags.end()) {
        return std::nullopt;
    }
    return singletonTags[tag];
}

void EntityManager::SetSingletonTag(std::string tag, EntityPtr ptr)
{
    if (!IsValid(ptr)) {
        return;
    }
    singletonTags[tag] = ptr;
}

void EntityManager::BroadcastEvent(HashedString eventName, EventArgs eventArgs)
{
    for (auto & e : liveEntities) {
        if (!e.isRemoved) {
            e.entity.FireEvent(eventName, eventArgs);
        }
    }
}

bool EntityManager::IsValid(EntityPtr ptr) const
{
    if (ptr.entityManager != this) {
        return false;
    }
    if (ptr.index > liveEntities.size()) {
        return false;
    }
    auto & entity = liveEntities[ptr.index];
    if (entity.generation != ptr.generation) {
        return false;
    }
    if (entity.isRemoved) {
        return false;
    }
    return true;
}