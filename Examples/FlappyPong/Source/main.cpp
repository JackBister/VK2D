#include <utility>
#include <vector>

#define REFLECT_IMPL
#include <Core/Reflect.h>
#undef REFLECT_IMPL

#include "BallComponent.h"
#include "PaddleComponent.h"

static BallComponent ballComponent;
static PaddleComponent paddleComponent;

extern "C" void __declspec(dllexport) LoadComponents() {}

extern "C" void __declspec(dllexport) UnloadComponents()
{
    Deserializable::Map()["BallComponent"] = nullptr;
    Deserializable::Map()["PaddleComponent"] = nullptr;
}
