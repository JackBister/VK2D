#include "Core/scene.h"

#include <atomic>
#include <cinttypes>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"
#include "rttr/registration.h"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Renderer.h"
#include "Core/sprite.h"
#include "Core/transform.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Scene>("Scene")
	.method("GetEntityByName", static_cast<Entity*(Scene::*)(std::string)>(&Scene::GetEntityByName))
	.method("BroadcastEvent", &Scene::BroadcastEvent);
}


Scene::Scene(std::string const& name, ResourceManager * resMan,	std::string const& fileName)
	: resourceManager(resMan)
{
	this->name = name;

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
		printf("[ERROR] Scene::LoadFile failed to open file %s\n", fileName.c_str());
		return;
	}
	auto const j = nlohmann::json::parse(serializedScene);

	if (j.find("input") != j.end()) {
		GameModule::DeserializeInput(j["input"].dump());
	}

	if (j.find("physics") != j.end()) {
		GameModule::DeserializePhysics(j["physics"].dump());
	}

	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resourceManager, je.dump()));
		tmp->scene = this;
		this->entities.push_back(tmp);
	}
}

void Scene::SerializeToFile(std::string const& filename)
{
	nlohmann::json j;
	j["input"] = nlohmann::json::parse(GameModule::SerializeInput());
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
		delete e;
	}
	entities = std::vector<Entity *>();
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

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities) {
		ep->FireEvent(ename, eas);
	}
}
