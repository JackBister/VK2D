#include "Core/entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"
#include "rttr/registration.h"

#include "Core/Components/component.h"
#include "Core/scene.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Entity>("Entity")
	.method("FireEvent", &Entity::FireEvent)
	.method("GetComponent", &Entity::GetComponent)
	.property("name", &Entity::name)
	.property("transform", &Entity::transform);
}

DESERIALIZABLE_IMPL(Entity)

void Entity::FireEvent(std::string ename, EventArgs args)
{
	for (auto const c : components) {
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
	for (auto const c : components) {
		if (c->type == type) {
			return c;
		}
	}
	return nullptr;
}

Deserializable * Entity::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * const mem = alloc.Allocate(sizeof(Entity));
	Entity * const ret = new (mem) Entity();
	auto const j = nlohmann::json::parse(str);
	ret->name = j["name"].get<std::string>();
	ret->transform = Transform::Deserialize(j["transform"].dump());
	auto const t = j["components"];
	for (auto const& js : j["components"]) {
		Component * const c = static_cast<Component *>(Deserializable::DeserializeString(resourceManager, js.dump(), alloc));
		c->entity = ret;
		ret->components.push_back(c);
	}
	return ret;
}
