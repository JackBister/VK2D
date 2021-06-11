#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "EntityId.h"
#include "EntityPtr.h"
#include "Util/DllExport.h"
#include "entity.h"

struct FreeSlot {
    size_t index;
    size_t generation;
};

struct LiveEntity {
    size_t generation;
    bool isRemoved;
    Entity entity;
};

class EAPI EntityManager
{
public:
    static std::string const IS_MAIN_CAMERA_TAG;
    static EntityManager * GetInstance();

    EntityPtr AddEntity(Entity e);
    void RemoveEntity(EntityPtr ptr);

    Entity * Get(EntityPtr ptr) const;

    EntityPtr First();
    EntityPtr Next(EntityPtr current);
    EntityPtr Prev(EntityPtr current);

    EntityPtr GetEntityById(EntityId id);

    // A "singleton tag" is a tag which can only be applied to a single entity at a time.
    // For example, only one entity can be the main camera.
    std::optional<EntityPtr> GetEntityBySingletonTag(std::string tag);
    void SetSingletonTag(std::string tag, EntityPtr ptr);

    void BroadcastEvent(HashedString eventName, EventArgs eventArgs);

private:
    bool IsValid(EntityPtr ptr) const;

    std::mutex liveEntitiesLock;
    std::vector<LiveEntity> liveEntities;
    std::deque<FreeSlot> freeSlots;
    std::unordered_map<std::string, EntityPtr> singletonTags;
    std::unordered_map<EntityId, EntityPtr> idToPtr;
};