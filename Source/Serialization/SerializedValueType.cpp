#include "SerializedValueType.h"

#include <cassert>

std::optional<SerializedValueType> SerializedValueTypeFromString(std::string s)
{
    if (s == "BOOL") {
        return SerializedValueType::BOOL;
    } else if (s == "DOUBLE") {
        return SerializedValueType::DOUBLE;
    } else if (s == "STRING") {
        return SerializedValueType::STRING;
    } else if (s == "OBJECT") {
        return SerializedValueType::OBJECT;
    } else if (s == "ARRAY") {
        return SerializedValueType::ARRAY;
    }
    return std::nullopt;
}

std::string SerializedValueTypeToString(SerializedValueType t)
{
    switch (t) {
    case SerializedValueType::BOOL:
        return "BOOL";
    case SerializedValueType::DOUBLE:
        return "DOUBLE";
    case SerializedValueType::STRING:
        return "STRING";
    case SerializedValueType::OBJECT:
        return "OBJECT";
    case SerializedValueType::ARRAY:
        return "ARRAY";
    default:
        assert(false);
        return "";
    }
}
