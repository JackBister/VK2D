#pragma once

#include <chrono>

struct Time
{
	float timeScale;

	void Start(float timeScale = 1.f);
	void Frame();
	float GetDeltaTime();
	std::chrono::high_resolution_clock::time_point GetLastTime();

private:
	float deltaTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	std::chrono::high_resolution_clock::time_point startTime;
};
