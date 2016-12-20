#include "scene.h"

#include <fstream>
#include <sstream>

#include "json.hpp"

#include "entity.h"
#include "luaindex.h"

#include "scene.h.generated.h"

Scene::Scene(ResourceManager * resMan, const std::string& name) noexcept
{
	//TODO: Default input and physicsworld
	this->name = name;
	this->resourceManager = resMan;
}

Scene::Scene(ResourceManager * resMan, const std::string& name, const std::vector<char>& input) noexcept
{
	using nlohmann::json;
	this->name = name;
//	std::string fileContent = std::string(input.data());
	std::string fileContent = std::string(input.begin(), input.end());
	json j = json::parse(fileContent);
	this->resourceManager = resMan;
	this->input = static_cast<Input *>(Deserializable::DeserializeString(resourceManager, j["input"].dump()));
	this->physicsWorld = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(resourceManager, j["physics"].dump()));
	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resourceManager, je.dump()));
		tmp->scene = this;
		this->entities.push_back(tmp);
	}
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
