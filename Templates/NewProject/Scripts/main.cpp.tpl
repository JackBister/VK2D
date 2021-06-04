#include <utility>
#include <vector>

#define REFLECT_IMPL
#include <Core/Reflect.h>
#undef REFLECT_IMPL
#include <Serialization/Deserializable.h>


extern "C" void __declspec(dllexport) LoadComponents() {}

extern "C" void __declspec(dllexport) UnloadComponents()
{
    Deserializable::RemoveByOwner("{% projectName %}");
}
