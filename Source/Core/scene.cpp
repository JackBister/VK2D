#include "scene.h"

#include <fstream>
#include <sstream>

#include "json.hpp"

#include "entity.h"
#include "luaindex.h"

#include "scene.h.generated.h"

using nlohmann::json;
using namespace std;

string SlurpFile(string fn)
{
	ifstream in = ifstream(fn);
	stringstream sstr;
	sstr << in.rdbuf();
	return sstr.str();
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

Scene * Scene::FromFile(string fn)
{
	Scene * ret = new Scene();
	string fileContent = SlurpFile(fn);
	json j = json::parse(fileContent);
	ret->input = static_cast<Input *>(Deserializable::DeserializeString(j["input"].dump()));
	ret->physicsWorld = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(j["physics"].dump()));
	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(je.dump()));
		tmp->scene = ret;
		ret->entities.push_back(tmp);
	}
	return ret;
}

