#include "entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

#include "luaindex.h"
#include "scene.h"

#include "entity.h.generated.h"

using nlohmann::json;
using namespace std;

void Entity::FireEvent(std::string ename, EventArgs args)
{
	for (auto& c : components) {
		if (c->isActive) {
			//Only send tick event if component::receiveTicks is true
			if (ename != "Tick" || c->receiveTicks) {
				c->OnEvent(ename, args);
			}
			if (ename == "EndPlay") {
				c->isActive = false;
			}
		}
	}
}

Component * Entity::GetComponent(std::string type) const
{
	for (auto c : components) {
		if (c->type == type) {
			return c;
		}
	}
	return nullptr;
}

Entity * Entity::Deserialize(string s)
{
	Entity * ret = new Entity();
	json j = json::parse(s);
	ret->name = j["name"].get<string>();
	ret->transform = Transform::Deserialize(j["transform"].dump());
	json t = j["components"];
	for (auto& js : j["components"]) {
		Component * c = Component::Deserialize(js.dump());
		c->entity = ret;
		ret->components.push_back(c);
	}
	return ret;
}
