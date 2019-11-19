#include "Core/Deserializable.h"

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("Deserializable");

Deserializable * Deserializable::Deserialize(SerializedObject const & obj)
{
    auto typeOpt = obj.GetString("type");
    if (!typeOpt.has_value()) {
        logger->Errorf("Key 'type' was not present when deserializing object.");
    }
    std::string type = typeOpt.value_or("MISSING_TYPE");
    Deserializable * ret = Map()[type](obj);
    ret->type = type;
    return ret;
}

std::unordered_map<std::string, std::function<Deserializable *(SerializedObject const &)>> & Deserializable::Map()
{
    static auto map = std::unordered_map<std::string, std::function<Deserializable *(SerializedObject const &)>>();
    return map;
}
