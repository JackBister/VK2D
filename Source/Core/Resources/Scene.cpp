#include "Scene.h"

#include <atomic>
#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Core/Components/CameraComponent.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Serialization/JsonSerializer.h"
#include "Core/Util/WatchFile.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/transform.h"

static const auto logger = Logger::Create("Scene");

static void ReadFile(std::string const & fileName, std::vector<std::string> & dlls, std::vector<Entity *> & entities)
{
    FILE * f = fopen(fileName.c_str(), "rb");
    std::string serializedSceneString;
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t length = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<char> buf(length + 1);
        fread(&buf[0], 1, length, f);
        serializedSceneString = std::string(buf.begin(), buf.end());
    } else {
        logger->Errorf("LoadFile failed to open fileName=%s", fileName.c_str());
        // TODO:
        return;
    }

    auto serializedScene = JsonSerializer().Deserialize(serializedSceneString).value();

    DeserializationContext context = {std::filesystem::path(fileName).parent_path()};

    auto dllsOpt = serializedScene.GetArray("dlls");
    if (dllsOpt.has_value()) {
        for (auto dll : dllsOpt.value()) {
            if (dll.index() != SerializedValue::STRING) {
                logger->Errorf("Unexpected non-string in dlls array in scene file.");
            }
            auto dllStr = std::get<std::string>(dll);
            dlls.push_back(dllStr);
            GameModule::LoadDLL((context.workingDirectory / dllStr).string());
        }
    }

    auto inputOpt = serializedScene.GetObject("input");
    if (inputOpt.has_value()) {
        auto input = inputOpt.value();
        auto keybindsOpt = input.GetArray("keybinds");
        for (auto kb : keybindsOpt.value()) {
            if (kb.index() != SerializedValue::OBJECT) {
                logger->Errorf("Unexpected non-object in keybinds array in scene file.");
            }
            auto keybind = std::get<SerializedObject>(kb);
            auto keys = keybind.GetArray("keys");
            for (auto k : keys.value()) {
                Input::AddKeybind(keybind.GetString("name").value(), strToKeycode[std::get<std::string>(k)]);
            }
        }
    }

    auto physicsOpt = serializedScene.GetObject("physics");
    if (physicsOpt.has_value()) {
        GameModule::DeserializePhysics(&context, physicsOpt.value());
    }

    auto serializedEntities = serializedScene.GetArray("entities");
    for (auto & e : serializedEntities.value()) {
        if (e.index() != SerializedValue::OBJECT) {
            logger->Errorf("Unexpected non-object in entities array in scene file.");
        }
        Entity * entity = static_cast<Entity *>(Deserializable::Deserialize(&context, std::get<SerializedObject>(e)));
        entities.push_back(entity);
        GameModule::AddEntity(entity);
    }
}

Scene * Scene::FromFile(std::string const & fileName)
{
    std::vector<std::string> dlls;
    std::vector<Entity *> entities;

    ReadFile(fileName, dlls, entities);

    auto ret = new Scene(fileName, dlls, entities);
    ResourceManager::AddResource(fileName, ret);
#if HOT_RELOAD_RESOURCES
    WatchFile(fileName, [fileName]() {
        logger->Infof("Scene file '%s' changed, will reload on next frame start", fileName.c_str());
        GameModule::OnFrameStart([fileName]() {
            auto scene = ResourceManager::GetResource<Scene>(fileName);
            if (scene == nullptr) {
                logger->Warnf("Scene file '%s' was changed but ResourceManager had no reference for it.",
                              fileName.c_str());
                return;
            }

            scene->Unload();
            ReadFile(fileName, scene->dlls, scene->entities);
        });
    });
#endif
    return ret;
}

void Scene::SerializeToFile(std::string const & filename)
{
    // TODO: input
    SerializedObject::Builder builder;
    builder.WithObject("physics", GameModule::SerializePhysics());

    std::vector<SerializedValue> serializedDlls;
    serializedDlls.reserve(dlls.size());
    for (auto const & dll : dlls) {
        serializedDlls.push_back(dll);
    }
    builder.WithArray("dlls", serializedDlls);

    std::vector<SerializedValue> serializedEntities;
    serializedEntities.reserve(entities.size());
    for (auto const e : entities) {
        serializedEntities.push_back(e->Serialize());
    }
    builder.WithArray("entities", serializedEntities);

    std::ofstream out(filename);
    out << JsonSerializer().Serialize(builder.Build());
}

void Scene::Unload()
{
    for (auto const e : entities) {
        GameModule::RemoveEntity(e);
        delete e;
    }
    entities.clear();
    for (auto const & dll : dlls) {
        GameModule::UnloadDLL(dll);
    }
    dlls.clear();
}

Scene::Scene(std::string const & fileName, std::vector<std::string> dlls, std::vector<Entity *> entities)
    : fileName(fileName), dlls(dlls), entities(entities)
{
}
