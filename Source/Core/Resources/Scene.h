#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/EntityPtr.h"
#include "Core/eventarg.h"
#include "Util/HashedString.h"

class Entity;
class EntityManager;
class SceneDeserializer;

class Scene : public Deserializable
{
public:
    friend class SceneDeserializer;

    static std::optional<Scene> Deserialize(DeserializationContext * deserializationContext,
                                            SerializedObject const & obj);
    static Scene * FromFile(std::string const &, EntityManager * entityManager);
    static std::unique_ptr<Scene> CreateNew(std::string const & name, DeserializationContext deserializationContext);

    Scene(std::string const & name, SerializedArray const & serializedEntities, std::vector<EntityPtr> liveEntities,
          DeserializationContext deserializationContext)
        : name(name), serializedEntities(serializedEntities), liveEntities(liveEntities),
          deserializationContext(deserializationContext)
    {
    }

    SerializedObject Serialize() const final override;
    void SerializeToFile(std::string const & filename);

    void Load(EntityManager * entityManager);
    void Unload(EntityManager * entityManager);

    inline std::string GetName() const { return name; }

    inline void AddEntity(EntityPtr entity) { liveEntities.push_back(entity); }
    void RemoveEntity(EntityPtr entity);

private:
    std::string name;

    DeserializationContext deserializationContext;
    // serializedEntities contains the serialized entities as they were when loaded from the file
    SerializedArray serializedEntities;
    // liveEntities contains the entities which are active in the game now.
    // TODO: There is some potential weirdness when editing a scene. When saving, the currently live entities will be
    // saved. Is this how it should work? Maybe.
    std::vector<EntityPtr> liveEntities;
};
