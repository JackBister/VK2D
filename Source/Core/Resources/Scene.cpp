#include "Scene.h"

#include <atomic>
#include <cinttypes>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"

#include "Core/Components/CameraComponent.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/transform.h"
#include "Core/Util/WatchFile.h"

static const auto logger = Logger::Create("Scene");

static void ReadFile(std::string const & fileName, std::vector<std::string> & dlls, std::vector<Entity *> & entities)
{
    FILE * f = fopen(fileName.c_str(), "rb");
    std::string serializedScene;
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t length = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<char> buf(length + 1);
        fread(&buf[0], 1, length, f);
        serializedScene = std::string(buf.begin(), buf.end());
    } else {
        logger->Errorf("LoadFile failed to open fileName=%s", fileName.c_str());
        // TODO:
        return;
    }
    auto const j = nlohmann::json::parse(serializedScene);

    if (j.find("dlls") != j.end()) {
        for (auto dll : j["dlls"]) {
            dlls.push_back(dll);
            GameModule::LoadDLL(dll);
        }
    }

    if (j.find("input") != j.end()) {
        auto input = j["input"];
        if (input.find("keybinds") != input.end()) {
            for (auto kb : input["keybinds"]) {
                for (auto k : kb["keys"]) {
                    Input::AddKeybind(kb["name"].get<std::string>(), strToKeycode[k]);
                }
            }
        }
    }

    if (j.find("physics") != j.end()) {
        GameModule::DeserializePhysics(j["physics"].dump());
    }

    for (auto & je : j["entities"]) {
        Entity * entity = static_cast<Entity *>(Deserializable::DeserializeString(je.dump()));
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
                logger->Warnf("Scene file '%s' was changed but ResourceManager had no reference for it.", fileName.c_str());
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
    nlohmann::json j;
    j["physics"] = nlohmann::json::parse(GameModule::SerializePhysics());

    std::vector<nlohmann::json> serializedEntities;
    serializedEntities.reserve(entities.size());
    for (auto const e : entities) {
        serializedEntities.push_back(nlohmann::json::parse(e->Serialize()));
    }

    j["entities"] = serializedEntities;

    std::ofstream out(filename);
    out << j.dump();
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
