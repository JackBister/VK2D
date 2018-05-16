#pragma once

#include <chrono>

namespace Time
{
	void Frame();
	float GetDeltaTime();
	float GetUnscaledDeltaTime();
	std::chrono::high_resolution_clock::time_point GetLastTime();
	void SetTimeScale(float timeScale);
	void Start(float timeScale = 1.f);
}
