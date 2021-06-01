#pragma once

#include <filesystem>
#include <functional>
#include <unordered_map>

#include "Core/Resources/Scene.h"

struct SceneChangeEvent {
    enum class Type { SCENE_UNLOADED, SCENE_LOADED };

    Type type;

    std::optional<std::filesystem::path> newScene;
};

class SceneManager
{
public:
    using SceneChangeListener = std::function<void(SceneChangeEvent)>;

    static SceneManager * GetInstance();

    SceneManager(EntityManager * entityManager);

    void UnloadCurrentScene();
    bool ChangeScene(std::filesystem::path newScenePath);

    // TODO: This doesn't feel great
    Scene * GetCurrentScene() { return currentScene.has_value() ? &currentScene.value() : nullptr; }

    void AddChangeListener(std::string const & key, SceneChangeListener listener) { changeListeners[key] = listener; }

    void RemoveChangeListener(std::string const & key)
    {
        if (changeListeners.find(key) != changeListeners.end()) {
            changeListeners.erase(changeListeners.find(key));
        }
    }

private:
    EntityManager * entityManager;

    std::optional<Scene> currentScene;

    std::unordered_map<std::string, SceneChangeListener> changeListeners;
};