#pragma once

#include "DeserializationContext.h"
#include "SerializedObject.h"
#include "SerializedObjectSchema.h"

class Deserializer
{
public:
    virtual SerializedObjectSchema GetSchema() = 0;

    virtual void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) = 0;
};
