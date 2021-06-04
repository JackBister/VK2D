#pragma once

#include <string>

#include "DeserializationContext.h"
#include "SerializedObject.h"
#include "SerializedObjectSchema.h"

class Deserializer
{
public:
    virtual std::string constexpr GetOwner() { return "Core"; }

    virtual SerializedObjectSchema GetSchema() = 0;

    virtual void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) = 0;
};
