#include "component.h"

#include "json.hpp"

using namespace std;
using nlohmann::json;

Component::~Component() {}

/*
unordered_map<string, Component*>& Component::ComponentMap()
{
	static auto map = unordered_map<string, Component *>();
	return map;
}

Component * Component::Deserialize(std::string s)
{
	json j = json::parse(s);
	//If your JSON is incorrect you will crash here. You deserve it.
	string type = j["type"];
	Component * ret = ComponentMap()[type]->Create(s);
	ret->type = type;
	return ret;
}
*/