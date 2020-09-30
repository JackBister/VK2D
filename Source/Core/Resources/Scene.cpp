#include "Scene.h"

#include <atomic>
#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Core/GameModule.h"
#include "Core/Input/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Serialization/JsonSerializer.h"
#include "Core/Util/WatchFile.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"

static const auto logger = Logger::Create("Scene");

static SerializedObjectSchema const SCENE_SCHEMA = SerializedObjectSchema(
    "Scene", {
                 SerializedPropertySchema("dlls", SerializedValueType::ARRAY, SerializedValueType::STRING),
                 SerializedPropertySchema("entities", SerializedValueType::ARRAY, SerializedValueType::OBJECT),
             });

class SceneDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return SCENE_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        std::vector<std::string> dlls;
        std::vector<Entity *> entities;

        auto dllsOpt = obj.GetArray("dlls");
        if (dllsOpt.has_value()) {
            for (auto dll : dllsOpt.value()) {
                auto dllStr = std::get<std::string>(dll);
                dlls.push_back(dllStr);
                GameModule::LoadDLL((ctx->workingDirectory / dllStr).string());
            }
        }

        auto inputOpt = obj.GetObject("input");
        if (inputOpt.has_value()) {
            auto input = inputOpt.value();
            auto keybindsOpt = input.GetArray("keybinds");
            for (auto kb : keybindsOpt.value()) {
                auto keybind = std::get<SerializedObject>(kb);
                auto keys = keybind.GetArray("keys");
                for (auto k : keys.value()) {
                    Input::AddKeybind(keybind.GetString("name").value(), strToKeycode[std::get<std::string>(k)]);
                }
            }
        }

        auto physicsOpt = obj.GetObject("physics");
        if (physicsOpt.has_value()) {
            GameModule::DeserializePhysics(ctx, physicsOpt.value());
        }

        auto serializedEntities = obj.GetArray("entities");
        for (auto & e : serializedEntities.value()) {
            Entity * entity = static_cast<Entity *>(Deserializable::Deserialize(ctx, std::get<SerializedObject>(e)));
            entities.push_back(entity);
            GameModule::AddEntity(entity);
        }

        return new Scene(dlls, entities);
    }
};

DESERIALIZABLE_IMPL(Scene, new SceneDeserializer())

static Scene * ReadFile(std::string const & fileName)
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
        return nullptr;
    }

    auto serializedScene = JsonSerializer().Deserialize(serializedSceneString).value();

    DeserializationContext context = {std::filesystem::path(fileName).parent_path()};

    return (Scene *)Deserializable::Deserialize(&context, serializedScene);
}

Scene * Scene::FromFile(std::string const & fileName)
{

    auto ret = ReadFile(fileName);
    if (!ret) {
        return nullptr;
    }
    ret->fileName = fileName;
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

            // This is pretty messy
            auto newScene = ReadFile(fileName);
            if (!newScene) {
                logger->Warnf("Failed to reload scene file=%s, will keep existing scene. See previous errors",
                              fileName.c_str());
                return;
            }
            newScene->fileName = fileName;
            scene->Unload();
            scene->dlls = newScene->dlls;
            scene->entities = newScene->entities;
            delete newScene;
        });
    });
#endif
    return ret;
}

std::unique_ptr<Scene> Scene::Create(std::string const & fileName)
{
    return std::unique_ptr<Scene>(new Scene(fileName, std::vector<std::string>(), std::vector<Entity *>()));
}

void Scene::RemoveEntity(Entity * entity)
{
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        if (*it == entity) {
            entities.erase(it);
            return;
        }
    }
}

SerializedObject Scene::Serialize() const
{
    // TODO: input
    SerializedObject::Builder builder;
    builder.WithString("type", "Scene");
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
    return builder.Build();
}

void Scene::SerializeToFile(std::string const & filename)
{
    SerializedObject serialized = Serialize();
    std::ofstream out(filename);
    out << JsonSerializer().Serialize(serialized);
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

Scene::Scene(std::vector<std::string> dlls, std::vector<Entity *> entities) : dlls(dlls), entities(entities) {}
