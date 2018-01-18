#include "Core/Deserializable.h"

#include <nlohmann/json.hpp>

Deserializable * Deserializable::DeserializeString(ResourceManager * resourceManager, std::string const& str)
{
	auto j = nlohmann::json::parse(str);
	//If your JSON is incorrect you will crash here. You deserve it.
	std::string type = j["type"];
 	Deserializable * ret = Map()[type](resourceManager, str);
	ret->type = type;
	return ret;
}

std::unordered_map <std::string, std::function<Deserializable *(ResourceManager *, std::string const&)>>& Deserializable::Map()
{
	static auto map = std::unordered_map <std::string, std::function<Deserializable *(ResourceManager *, std::string const&)>>();
	return map;
}
