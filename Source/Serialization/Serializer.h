#pragma once

#include <optional>

#include "SerializedValue.h"

struct SerializeOptions {
    bool prettyPrint = false;
};

class Serializer
{
public:
    virtual std::optional<SerializedObject> Deserialize(std::string const &) = 0;
    virtual std::string Serialize(SerializedObject const &, SerializeOptions options = {}) = 0;
};
