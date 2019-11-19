#pragma once

#include "Serializer.h"

class JsonSerializer : public Serializer
{
public:
    virtual std::optional<SerializedObject> Deserialize(std::string const &) final override;
    virtual std::string Serialize(SerializedObject const &) final override;
};
