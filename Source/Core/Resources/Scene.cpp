#include "Scene.h"

#include <filesystem>
#include <fstream>

#include "Core/EntityManager.h"
#include "Core/GameModule.h"
#include "Core/Resources/ResourceManager.h"
#include "Logging/Logger.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/SchemaValidator.h"
#include "Util/WatchFile.h"

static const auto logger = Logger::Create("Scene");

static SerializedObjectSchema const SCENE_SCHEMA =
    SerializedObjectSchema("Scene", {SerializedPropertySchema::Required("name", SerializedValueType::STRING),
                                     SerializedPropertySchema::RequiredObjectArray("entities", "Entity")});

class SceneDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return SCENE_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        return new Scene(obj.GetString("name").value(), obj.GetArray("entities").value(), {}, *ctx);
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
        logger.Error("LoadFile failed to open fileName={}", fileName);
        return nullptr;
    }

    auto serializedScene = JsonSerializer().Deserialize(serializedSceneString).value();

    DeserializationContext context = {std::filesystem::path(fileName).parent_path()};

    return (Scene *)Deserializable::Deserialize(&context, serializedScene);
}

std::optional<Scene> Scene::Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj)
{
    auto validationResult = SchemaValidator::Validate(SCENE_SCHEMA, obj);
    if (!validationResult.isValid) {
        logger.Warn("Failed to deserialize Scene from SerializedObject, validation failed. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger.Warn("\t{}: {}", err.first, err.second);
        }
        return std::nullopt;
    }
    return Scene(obj.GetString("name").value(), obj.GetArray("entities").value(), {}, *deserializationContext);
}

Scene * Scene::FromFile(std::string const & fileName, EntityManager * entityManager)
{

    auto ret = ReadFile(fileName);
    if (!ret) {
        return nullptr;
    }
    ResourceManager::AddResource(fileName, ret);
#if HOT_RELOAD_RESOURCES
    WatchFile(fileName, [fileName, entityManager]() {
        logger.Info("Scene file '{}' changed, will reload on next frame start", fileName);
        GameModule::OnFrameStart([fileName, entityManager]() {
            auto scene = ResourceManager::GetResource<Scene>(fileName);
            if (scene == nullptr) {
                logger.Warn("Scene file '{}' was changed but ResourceManager had no reference for it.", fileName);
                return;
            }

            // This is pretty messy
            auto newScene = ReadFile(fileName);
            if (!newScene) {
                logger.Warn("Failed to reload scene file={}, will keep existing scene. See previous errors", fileName);
                return;
            }
            // TODO: This is really dumb
            scene->Unload(entityManager);
            scene->name = newScene->name;
            scene->serializedEntities = newScene->serializedEntities;
            scene->liveEntities = newScene->liveEntities;
            scene->Load(entityManager);
            delete newScene;
        });
    });
#endif
    return ret;
}

std::unique_ptr<Scene> Scene::CreateNew(std::string const & name, DeserializationContext deserializationContext)
{
    return std::unique_ptr<Scene>(new Scene(name, {}, {}, deserializationContext));
}

void Scene::RemoveEntity(EntityPtr entity)
{
    for (auto it = liveEntities.begin(); it != liveEntities.end(); ++it) {
        if (*it == entity) {
            liveEntities.erase(it);
            return;
        }
    }
}

SerializedObject Scene::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", "Scene").WithString("name", name);

    std::vector<SerializedValue> newSerializedEntities;
    newSerializedEntities.reserve(liveEntities.size());
    for (auto const e : liveEntities) {
        auto entity = e.Get();
        if (entity) {
            newSerializedEntities.push_back(entity->Serialize());
        }
    }
    builder.WithArray("entities", newSerializedEntities);
    return builder.Build();
}

void Scene::SerializeToFile(std::string const & filename)
{
    SerializedObject serialized = Serialize();
    std::ofstream out(filename);
    out << JsonSerializer().Serialize(serialized, {.prettyPrint = true});
}

void Scene::Load(EntityManager * entityManager)
{
    for (auto const e : serializedEntities) {
        if (e.GetType() != SerializedValueType::OBJECT) {
            logger.Error("Failed to load entity from scene with name={}, value in entities array was not an object",
                         name);
            continue;
        }
        auto entityOpt = Entity::Deserialize(&deserializationContext, std::get<SerializedObject>(e));
        if (!entityOpt.has_value()) {
            logger.Error("Failed to load entity from scene with name={}, deserialization failed", name);
        } else {
            liveEntities.push_back(entityManager->AddEntity(entityOpt.value()));
        }
    }
}

void Scene::Unload(EntityManager * entityManager)
{
    for (auto const e : liveEntities) {
        entityManager->RemoveEntity(e);
    }
    liveEntities.clear();
}
