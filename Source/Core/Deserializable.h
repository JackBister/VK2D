#pragma once
#include <filesystem>
#include <unordered_map>

#include "Core/DllExport.h"
#include "Core/HashedString.h"
#include "Core/Serialization/SerializedValue.h"

struct DeserializationContext {
    std::filesystem::path workingDirectory;
};

class EAPI Deserializable
{
public:
    virtual SerializedObject Serialize() const = 0;

    /*
     * Deserializes an object. The object must contain the key "type" with a string value which must be present as
     * a key in Deserializable::Map
     */
    static Deserializable * Deserialize(DeserializationContext * deserializationContext, SerializedObject const & obj);

    static std::unordered_map<std::string,
                              std::function<Deserializable *(DeserializationContext *, SerializedObject const &)>> &
    Map();

    // TODO: Remove?
    std::string type;
};

#ifdef VK2D_DLL
#define DESERIALIZABLE_IMPL(str, fn)
#elif defined(VK2D_LIB)
#define DESERIALIZABLE_IMPL(str, fn)                                                                                   \
    static bool _##str##Instantiate()                                                                                  \
    {                                                                                                                  \
        Deserializable::Map()[#str] = fn;                                                                              \
        return true;                                                                                                   \
    }                                                                                                                  \
    extern "C" bool _##str##StaticInst = _##str##Instantiate();
#else
#define DESERIALIZABLE_IMPL(str, fn)                                                                                   \
    static bool _##str##Instantiate()                                                                                  \
    {                                                                                                                  \
        Deserializable::Map()[#str] = fn;                                                                              \
        return true;                                                                                                   \
    }                                                                                                                  \
    static bool _##str##StaticInst = _##str##Instantiate();
#endif
