#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "SerializedObject.h"

using SerializedArray = std::vector<SerializedValue>;

class SerializedValue : public std::variant<bool, double, std::string, SerializedObject, SerializedArray>
{
public:
    enum Type { BOOL = 0, DOUBLE = 1, STRING = 2, OBJECT = 3, ARRAY = 4 };

    using std::variant<bool, double, std::string, SerializedObject, SerializedArray>::variant;
};
