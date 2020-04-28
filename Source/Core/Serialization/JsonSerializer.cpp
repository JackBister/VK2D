﻿#include "JsonSerializer.h"

#include <nlohmann/json.hpp>

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("JsonDeserializer");

static SerializedObject DeserializeObject(nlohmann::json j);

static SerializedArray DeserializeArray(nlohmann::json j)
{
    SerializedArray ret;
    auto arr = j.get<std::vector<nlohmann::json>>();
    for (auto const & element : arr) {
        if (element.is_boolean()) {
            ret.push_back(element.get<bool>());
        } else if (element.is_number()) {
            ret.push_back(element.get<double>());
        } else if (element.is_string()) {
            ret.push_back(element.get<std::string>());
        } else if (element.is_object()) {
            ret.push_back(DeserializeObject(element));
        } else if (element.is_array()) {
            ret.push_back(DeserializeArray(element));
        } else {
            logger->Warnf("Unknown element type in JSON array. arrayString='%s'", j.dump().c_str());
        }
    }
    return ret;
}

static SerializedObject DeserializeObject(nlohmann::json j)
{
    SerializedObject::Builder builder;
    for (auto const & element : j.items()) {
        if (element.value().is_boolean()) {
            builder.WithBool(element.key(), element.value());
        } else if (element.value().is_number()) {
            builder.WithNumber(element.key(), element.value());
        } else if (element.value().is_string()) {
            builder.WithString(element.key(), element.value());
        } else if (element.value().is_object()) {
            builder.WithObject(element.key(), DeserializeObject(element.value()));
        } else if (element.value().is_array()) {
            builder.WithArray(element.key(), DeserializeArray(element.value()));
        } else {
            logger->Warnf("Unknown element type in JSON object. objectString='%s'", j.dump().c_str());
        }
    }
    return builder.Build();
}

std::optional<SerializedObject> JsonSerializer::Deserialize(std::string const & str)
{
    auto j = nlohmann::json::parse(str);

    if (!j.is_object()) {
        logger->Warnf("Attempt to deserialize non-object, will return empty optional. jsonString='%s'", str.c_str());
        return {};
    }

    return DeserializeObject(j);
}

static nlohmann::json SerializeObject(SerializedObject const & obj);

static nlohmann::json SerializeArray(SerializedArray const & arr)
{
    nlohmann::json ret = nlohmann::json::array();

    for (size_t i = 0; i < arr.size(); ++i) {
        auto const & element = arr[i];
        if (element.index() == SerializedValue::BOOL) {
            ret.push_back(std::get<bool>(element));
        } else if (element.index() == SerializedValue::DOUBLE) {
            ret.push_back(std::get<double>(element));
        } else if (element.index() == SerializedValue::STRING) {
            ret.push_back(std::get<std::string>(element));
        } else if (element.index() == SerializedValue::OBJECT) {
            ret.push_back(SerializeObject(std::get<SerializedObject>(element)));
        } else if (element.index() == SerializedValue::ARRAY) {
            ret.push_back(SerializeArray(std::get<SerializedArray>(element)));
        } else {
            logger->Warnf("Unknown value type (index=%d) for idx=%zu when serializing object.", element.index(), i);
        };
    }

    return ret;
}

static nlohmann::json SerializeObject(SerializedObject const & obj)
{
    nlohmann::json j = nlohmann::json::object();

    for (auto kv : obj.GetValues()) {
        if (kv.second.index() == SerializedValue::BOOL) {
            j[kv.first] = std::get<bool>(kv.second);
        } else if (kv.second.index() == SerializedValue::DOUBLE) {
            j[kv.first] = std::get<double>(kv.second);
        } else if (kv.second.index() == SerializedValue::STRING) {
            j[kv.first] = std::get<std::string>(kv.second);
        } else if (kv.second.index() == SerializedValue::OBJECT) {
            j[kv.first] = SerializeObject(std::get<SerializedObject>(kv.second));
        } else if (kv.second.index() == SerializedValue::ARRAY) {
            j[kv.first] = SerializeArray(std::get<SerializedArray>(kv.second));
        } else {
            logger->Warnf("Unknown value type (index=%d) for key=%s when serializing object.",
                          kv.second.index(),
                          kv.first.c_str());
        };
    }

    return j;
}

std::string JsonSerializer::Serialize(SerializedObject const & obj)
{
    return SerializeObject(obj).dump();
}
