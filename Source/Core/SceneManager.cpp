#include "SceneManager.h"

#include "Console/Console.h"
#include "Core/EntityManager.h"
#include "Logging/Logger.h"
#include "Serialization/JsonSerializer.h"
#include "Util/DefaultFileSlurper.h"

static auto const logger = Logger::Create("SceneManager");

SceneManager * SceneManager::GetInstance()
{
    static SceneManager singletonSceneManager(EntityManager::GetInstance());
    return &singletonSceneManager;
}

SceneManager::SceneManager(EntityManager * entityManager) : entityManager(entityManager)
{

    CommandDefinition dumpSceneCommand(
        "scene_dump", "scene_dump <to_file> - Dumps the scene into <to_file>", 1, [this](auto args) {
            auto toFile = args[0];
            if (currentScene.has_value()) {
                currentScene.value().SerializeToFile(toFile);
            }
        });
    Console::RegisterCommand(dumpSceneCommand);

    CommandDefinition loadSceneCommand(
        "scene_load", "scene_load <filename> - Loads the scene defined in the given file.", 1, [this](auto args) {
            auto fileName = args[0];
            ChangeScene(std::filesystem::path(fileName));
        });
    Console::RegisterCommand(loadSceneCommand);

    CommandDefinition unloadSceneCommand(
        "scene_unload", "scene_unload - Unloads the scene.", 0, [this](auto args) { UnloadCurrentScene(); });
    Console::RegisterCommand(unloadSceneCommand);
}

void SceneManager::UnloadCurrentScene()
{
    if (currentScene.has_value()) {
        currentScene.value().Unload(entityManager);
        currentScene = std::nullopt;
        for (auto & kv : changeListeners) {
            kv.second({.type = SceneChangeEvent::Type::SCENE_UNLOADED});
        }
    }
}

bool SceneManager::ChangeScene(std::filesystem::path newScenePath)
{
    logger.Info("Changing scene to new scene with path={}", newScenePath);
    DefaultFileSlurper slurper;
    JsonSerializer jsonSerializer;
    auto fileContent = slurper.SlurpFile(newScenePath.string());
    auto serializedSceneOpt = jsonSerializer.Deserialize(fileContent);
    if (!serializedSceneOpt.has_value()) {
        logger.Error("Failed to deserialize JSON from new Scene with path={}. Will not change scene", newScenePath);
        return false;
    }
    DeserializationContext ctx{.workingDirectory = newScenePath.parent_path()};
    auto newSceneOpt = Scene::Deserialize(&ctx, serializedSceneOpt.value());
    if (!newSceneOpt.has_value()) {
        logger.Error(
            "Failed to deserialize from SerializedObject to Scene for Scene with path={}. Will not change scene",
            newScenePath);
        return false;
    }
    if (currentScene.has_value()) {
        currentScene.value().Unload(entityManager);
        for (auto & kv : changeListeners) {
            kv.second({.type = SceneChangeEvent::Type::SCENE_UNLOADED});
        }
    }
    currentScene = newSceneOpt.value();
    currentScene.value().Load(entityManager);

    for (auto & kv : changeListeners) {
        kv.second({.type = SceneChangeEvent::Type::SCENE_LOADED, .newScene = newScenePath});
    }
    return true;
}
