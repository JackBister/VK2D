#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "SerializedObject.h"
#include "SerializedValueType.h"

using SerializedArray = std::vector<SerializedValue>;

class SerializedValue : public std::variant<bool, double, std::string, SerializedObject, SerializedArray>
{
public:
    using std::variant<bool, double, std::string, SerializedObject, SerializedArray>::variant;

    inline SerializedValueType GetType() const { return (SerializedValueType)index(); }
};
