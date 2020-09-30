#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/HashedString.h"
#include "Core/eventarg.h"

class Entity;
class SceneDeserializer;

class Scene : public Deserializable
{
public:
    friend class SceneDeserializer;

    static Scene * FromFile(std::string const &);
    static std::unique_ptr<Scene> Create(std::string const & fileName);

    SerializedObject Serialize() const final override;
    void SerializeToFile(std::string const & filename);
    void Unload();

    inline std::string GetFileName() const { return fileName; }

    inline void AddEntity(Entity * entity) { entities.push_back(entity); }
    void RemoveEntity(Entity * entity);

private:
    Scene(std::string const & fileName, std::vector<std::string> dlls, std::vector<Entity *> entities);
    Scene(std::vector<std::string> dlls, std::vector<Entity *> entities);

    std::string fileName;
    std::vector<std::string> dlls;
    std::vector<Entity *> entities;
};
