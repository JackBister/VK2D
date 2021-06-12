#include "Core/dtime.h"

#include <ThirdParty/optick/src/optick.h>

namespace Time
{
using std::chrono::high_resolution_clock;
float unscaledDeltaTime;
float timeScale;
std::chrono::high_resolution_clock::time_point lastTime;
std::chrono::high_resolution_clock::time_point startTime;

void Start(float ts)
{
    timeScale = ts;
    startTime = high_resolution_clock::now();
    lastTime = startTime;
}

void Frame()
{
    OPTICK_EVENT();
    high_resolution_clock::time_point currTime = high_resolution_clock::now();
    unscaledDeltaTime = std::chrono::duration<float>(currTime - lastTime).count();
    lastTime = currTime;
}

float GetDeltaTime()
{
    return timeScale * unscaledDeltaTime;
}

float GetUnscaledDeltaTime()
{
    return unscaledDeltaTime;
}

std::chrono::high_resolution_clock::time_point GetLastTime()
{
    return lastTime;
}
void SetTimeScale(float scale)
{
    timeScale = scale;
}
}
