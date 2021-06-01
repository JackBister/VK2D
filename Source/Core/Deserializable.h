﻿#pragma once
#include <filesystem>
#include <unordered_map>

#include "Core/Serialization/Deserializer.h"
#include "Core/Serialization/SerializedValue.h"
#include "Util/DllExport.h"
#include "Util/HashedString.h"

struct DeserializationContext;

class EAPI Deserializable
{
public:
    virtual SerializedObject Serialize() const = 0;

    /*
     * Deserializes an object. The object must contain the key "type" with a string value which must be present as
     * a key in Deserializable::Map
     */
    static void * Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj);

    static std::optional<SerializedObjectSchema> GetSchema(std::string const & type);

    static std::unordered_map<std::string, Deserializer *> & Map();

    std::string type;
};

//#ifdef VK2D_DLL
//#define DESERIALIZABLE_IMPL(str, deserializer)
//#elif defined(VK2D_LIB)
#ifdef VK2D_LIB
#define DESERIALIZABLE_IMPL(str, deserializer)                                                                         \
    static bool _##str##Instantiate()                                                                                  \
    {                                                                                                                  \
        Deserializable::Map()[#str] = deserializer;                                                                    \
        return true;                                                                                                   \
    }                                                                                                                  \
    extern "C" bool _##str##StaticInst = _##str##Instantiate();
#else
#define DESERIALIZABLE_IMPL(str, deserializer)                                                                         \
    static bool _##str##Instantiate()                                                                                  \
    {                                                                                                                  \
        Deserializable::Map()[#str] = deserializer;                                                                    \
        return true;                                                                                                   \
    }                                                                                                                  \
    static bool _##str##StaticInst = _##str##Instantiate();
#endif
