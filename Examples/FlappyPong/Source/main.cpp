#include <utility>
#include <vector>

#include "BallComponent.h"
#include "PaddleComponent.h"

static BallComponent ballComponent;
static PaddleComponent paddleComponent;

extern "C" void __declspec(dllexport) LoadComponents()
{
	Deserializable::Map()["BallComponent"] = &ballComponent;
	Deserializable::Map()["PaddleComponent"] = &paddleComponent;
}

extern "C" void __declspec(dllexport) UnloadComponents()
{
	Deserializable::Map()["BallComponent"] = nullptr;
	Deserializable::Map()["PaddleComponent"] = nullptr;
}
