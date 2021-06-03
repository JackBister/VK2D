#include "ProjectManager.h"

#include "Core/DllManager.h"
#include "Core/Input/Input.h"
#include "Core/SceneManager.h"
#include "Core/physicsworld.h"
#include "Logging/Logger.h"
#include "Serialization/JsonSerializer.h"
#include "Util/DefaultFileSlurper.h"

static auto const logger = Logger::Create("ProjectManager");

ProjectManager * ProjectManager::GetInstance()
{
    static ProjectManager singletonProjectManager(
        DllManager::GetInstance(), SceneManager::GetInstance(), PhysicsWorld::GetInstance());
    return &singletonProjectManager;
}

bool ProjectManager::ChangeProject(std::filesystem::path newProjectPath)
{
    logger->Infof("Changing project to project with path=%ls", newProjectPath.c_str());
    DefaultFileSlurper slurper;
    JsonSerializer jsonSerializer;
    auto projectFile = slurper.SlurpFile(newProjectPath.string());
    auto serializedProject = jsonSerializer.Deserialize(projectFile);
    DeserializationContext projectContext{.workingDirectory = newProjectPath.parent_path()};
    if (!serializedProject.has_value()) {
        logger->Severef("Failed to read project JSON with filename=%ls", newProjectPath.c_str());
        return false;
    }
    auto projectOpt = Project::Deserialize(&projectContext, serializedProject.value());
    if (!projectOpt.has_value()) {
        logger->Severef(
            "Failed to deserialize project with filename=%ls, there is hopefully logging above explaining why",
            newProjectPath.c_str());
        return false;
    }
    auto project = projectOpt.value();
    project.Load({.deserializationContext = &projectContext,
                  .AddKeybind = Input::AddKeybind,
                  .RemoveAllKeybindings = Input::RemoveAllKeybindings,
                  .dllManager = dllManager,
                  .sceneManager = sceneManager,
                  .physicsWorld = physicsWorld});

    for (auto & kv : changeListeners) {
        kv.second(newProjectPath);
    }
    return true;
}