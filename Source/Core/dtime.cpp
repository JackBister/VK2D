#include "Core/dtime.h"

using std::chrono::high_resolution_clock;

void Time::Start(float ts)
{
	timeScale = ts;
	startTime = high_resolution_clock::now();
	lastTime = startTime;
}

void Time::Frame()
{
	high_resolution_clock::time_point currTime = high_resolution_clock::now();
	deltaTime = timeScale * std::chrono::duration<float>(currTime - lastTime).count();
	lastTime = currTime;
}

float Time::GetDeltaTime()
{
	return deltaTime;
}
