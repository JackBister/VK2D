#include "Deserializable.h"

#include "Deserializer.h"
#include "Logging/Logger.h"
#include "SchemaValidator.h"

static const auto logger = Logger::Create("Deserializable");

void * Deserializable::Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj)
{
    auto typeOpt = obj.GetString("type");
    if (!typeOpt.has_value()) {
        logger->Errorf("Key 'type' was not present when deserializing object.");
        return nullptr;
    }
    std::string type = typeOpt.value();
    auto m = Map();
    auto found = m.find(type);
    if (found == m.end()) {
        logger->Errorf("No deserializer found for type=%s", type.c_str());
        return nullptr;
    }
    auto deserializer = found->second;
    auto validationResult = SchemaValidator::Validate(deserializer->GetSchema(), obj);
    if (!validationResult.isValid) {
        logger->Errorf("Failed to deserialize object of type=%s, object does not match schema. Errors:", type.c_str());
        for (auto err : validationResult.propertyErrors) {
            logger->Errorf("%s: %s", err.first.c_str(), err.second.c_str());
        }
        return nullptr;
    }
    void * ret = Map()[type]->Deserialize(deserializationContext, obj);
    return ret;
}

std::optional<SerializedObjectSchema> Deserializable::GetSchema(std::string const & type)
{
    auto m = Map();
    auto it = m.find(type);
    if (it == m.end()) {
        return {};
    }
    return it->second->GetSchema();
}

std::unordered_map<std::string, Deserializer *> & Deserializable::Map()
{
    static auto map = std::unordered_map<std::string, Deserializer *>();
    return map;
}

void Deserializable::RemoveByOwner(std::string const & owner)
{
    if (owner == "Core") {
        logger->Errorf("Attempt to RemoveByOwner with owner=Core. Will not do that.");
        return;
    }
    auto & m = Map();
    std::vector<std::string> toRemove;
    for (auto & kv : m) {
        if (kv.second->GetOwner() == owner) {
            toRemove.push_back(kv.first);
        }
    }

    for (auto & k : toRemove) {
        m.erase(k);
    }
}
