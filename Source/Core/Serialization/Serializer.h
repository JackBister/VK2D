#pragma once

#include <optional>

#include "SerializedValue.h"

class Serializer
{
public:
    virtual std::optional<SerializedObject> Deserialize(std::string const &) = 0;
    virtual std::string Serialize(SerializedObject const &) = 0;
};
