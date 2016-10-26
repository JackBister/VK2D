#include "Core/Deserializable.h"

#include <json.hpp>

Deserializable * Deserializable::DeserializeString(ResourceManager * resourceManager, const std::string& str, Allocator& alloc)
{
	using nlohmann::json;
	json j = json::parse(str);
	//If your JSON is incorrect you will crash here. You deserve it.
	std::string type = j["type"];
	Deserializable * ret = Map()[type]->Deserialize(resourceManager, str, alloc);
	ret->type = type;
	return ret;
}

std::unordered_map<std::string, const Deserializable *>& Deserializable::Map()
{
	static auto map = std::unordered_map<std::string, const Deserializable *>();
	return map;
}
