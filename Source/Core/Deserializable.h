#pragma once
#include <unordered_map>

#include "Core/Allocator.h"
#include "Core/DllExport.h"
#include "Core/HashedString.h"

class EAPI Deserializable
{
public:
    /*
            Serializes an object into a format suitable for storing in a scene file (currently JSON)
    */
    virtual std::string Serialize() const = 0;

    /*
            Deserializes a string of unknown type.
    */
    static Deserializable * DeserializeString(std::string const & str);

    static std::unordered_map<std::string, std::function<Deserializable *(std::string const &)>> & Map();

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
