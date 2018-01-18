#include <utility>
#include <vector>

#include "BallComponent.h"
#include "PaddleComponent.h"

static BallComponent ballComponent;
static PaddleComponent paddleComponent;

extern "C" void __declspec(dllexport) LoadComponents()
{
	Deserializable::Map()["BallComponent"] = &BallComponent::s_Deserialize;
	Deserializable::Map()["PaddleComponent"] = &PaddleComponent::s_Deserialize;
}

extern "C" void __declspec(dllexport) UnloadComponents()
{
	Deserializable::Map()["BallComponent"] = nullptr;
	Deserializable::Map()["PaddleComponent"] = nullptr;
}
