#include "Core/scene.h"

#include <atomic>
#include <cinttypes>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/physicsworld.h"
#include "Core/transform.h"

static const auto logger = Logger::Create("Scene");

Scene::Scene(std::string const& fileName)
{
	LoadFile(fileName);
}

void Scene::LoadFile(std::string const& fileName)
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
		return;
	}
	auto const j = nlohmann::json::parse(serializedScene);

	if (j.find("dlls") != j.end()) {
		for (auto fn : j["dlls"]) {
			dllFileNames.push_back(fn);
			GameModule::LoadDLL(fn);
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

	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(je.dump()));
		this->entities.push_back(tmp);
		GameModule::AddEntity(tmp);
	}
}

void Scene::SerializeToFile(std::string const& filename)
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
	}
	entities = std::vector<Entity *>();
	for (auto const& fn : dllFileNames) {
		GameModule::UnloadDLL(fn);
	}
	dllFileNames = std::vector<std::string>();
}

Entity * Scene::GetEntityByName(std::string name)
{
	for (auto& ep : entities) {
		if (ep->name == name) {
			return ep;
		}
	}
	return nullptr;
}

void Scene::BroadcastEvent(HashedString ename, EventArgs eas)
{
	for (auto& ep : entities) {
		ep->FireEvent(ename, eas);
	}
}
