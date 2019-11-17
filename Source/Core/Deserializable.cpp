#include "Core/Deserializable.h"

#include <nlohmann/json.hpp>

Deserializable * Deserializable::DeserializeString(std::string const & str)
{
    auto j = nlohmann::json::parse(str);
    // If your JSON is incorrect you will crash here. You deserve it.
    std::string type = j["type"];
    Deserializable * ret = Map()[type](str);
    ret->type = type;
    return ret;
}

std::unordered_map<std::string, std::function<Deserializable *(std::string const &)>> & Deserializable::Map()
{
    static auto map = std::unordered_map<std::string, std::function<Deserializable *(std::string const &)>>();
    return map;
}
