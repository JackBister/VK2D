#pragma once

#include <optional>
#include <string>

enum class SerializedValueType { BOOL = 0, DOUBLE = 1, STRING = 2, OBJECT = 3, ARRAY = 4 };

std::optional<SerializedValueType> SerializedValueTypeFromString(std::string s);
std::string SerializedValueTypeToString(SerializedValueType t);
