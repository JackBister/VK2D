#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Core/Deserializable.h"
#include "Core/Input/Keybinding.h"
#include "Core/Reflect.h"

class EntityManager;
class DllManager;
class PhysicsWorld;
class Scene;
class SceneManager;

struct ProjectLoadContext {
    DeserializationContext * const deserializationContext;

    std::function<void(std::string, Keycode)> const AddKeybind;
    std::function<void()> const RemoveAllKeybindings;

    DllManager * dllManager;
    SceneManager * sceneManager;
    PhysicsWorld * physicsWorld;
};

class Project : public Deserializable
{
public:
    static std::optional<Project> Deserialize(DeserializationContext * deserializationContext,
                                              SerializedObject const & obj);

    Project(std::string name, std::filesystem::path startingScene, glm::vec3 startingGravity,
            std::vector<std::filesystem::path> dlls, std::vector<Keybinding> defaultKeybindings);

    SerializedObject Serialize() const override;

    void Load(ProjectLoadContext ctx);

    std::string const & GetName() const { return name; }
    std::filesystem::path const & GetStartingScene() const { return startingScene; }
    glm::vec3 const & GetStartingGravity() const { return startingGravity; }
    std::vector<std::filesystem::path> const & GetDlls() const { return dlls; }
    std::vector<Keybinding> const & GetDefaultKeybindings() const { return defaultKeybindings; }

    REFLECT();

private:
    std::string name;
    std::filesystem::path startingScene;
    glm::vec3 startingGravity;
    std::vector<std::filesystem::path> dlls;
    std::vector<Keybinding> defaultKeybindings;
};
