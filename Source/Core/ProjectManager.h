#pragma once

#include <filesystem>
#include <unordered_map>

#include "Core/Resources/Project.h"

class DllManager;
class PhysicsWorld;
class SceneManager;

class ProjectManager
{
public:
    using ProjectChangeListener = std::function<void(std::filesystem::path newScene)>;

    static ProjectManager * GetInstance();

    ProjectManager(DllManager * dllManager, SceneManager * sceneManager, PhysicsWorld * physicsWorld)
        : dllManager(dllManager), sceneManager(sceneManager), physicsWorld(physicsWorld)
    {
    }

    bool ChangeProject(std::filesystem::path newProjectPath);

    void AddChangeListener(std::string const & key, ProjectChangeListener listener) { changeListeners[key] = listener; }

    void RemoveChangeListener(std::string const & key)
    {
        if (changeListeners.find(key) != changeListeners.end()) {
            changeListeners.erase(changeListeners.find(key));
        }
    }

private:
    DllManager * dllManager;
    SceneManager * sceneManager;
    PhysicsWorld * physicsWorld;

    std::unordered_map<std::string, ProjectChangeListener> changeListeners;
};