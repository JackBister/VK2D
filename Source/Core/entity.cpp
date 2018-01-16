#include "Core/entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"

#include "Core/Components/component.h"
#include "Core/scene.h"

DESERIALIZABLE_IMPL(Entity)

void Entity::FireEvent(std::string ename, EventArgs args)
{
	for (auto const& c : components) {
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
	for (auto const& c : components) {
		if (c->type == type) {
			return c.get();
		}
	}
	return nullptr;
}

Deserializable * Entity::Deserialize(ResourceManager * resourceManager, std::string const& str) const
{
	Entity * const ret = new Entity();
	auto const j = nlohmann::json::parse(str);
	ret->name = j["name"].get<std::string>();
	ret->transform = Transform::Deserialize(j["transform"].dump());
	auto const t = j["components"];
	for (auto const& js : j["components"]) {
		Component * const c = static_cast<Component *>(Deserializable::DeserializeString(resourceManager, js.dump()));
		c->entity = ret;
		ret->components.emplace_back(std::move(c));
	}
	return ret;
}

std::string Entity::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	j["name"] = name;
	j["transform"] = nlohmann::json::parse(transform.Serialize());

	std::vector<nlohmann::json> serializedComponents;
	for (auto const& c : components) {
		serializedComponents.push_back(nlohmann::json::parse(c->Serialize()));
	}

	j["components"] = serializedComponents;

	return j.dump();
}
