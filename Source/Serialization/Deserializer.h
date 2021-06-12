#pragma once

#include <string>

#include "SerializedObject.h"
#include "SerializedObjectSchema.h"

struct DeserializationContext;

class Deserializer
{
public:
    virtual std::string constexpr GetOwner() { return "Core"; }

    virtual SerializedObjectSchema GetSchema() = 0;

    virtual void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) = 0;
};
