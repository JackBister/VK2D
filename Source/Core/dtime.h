#pragma once

#include <chrono>

class Time
{
public:
	void Frame();
	float GetDeltaTime();
	std::chrono::high_resolution_clock::time_point GetLastTime();
	void Start(float timeScale = 1.f);

	float timeScale;
private:
	float deltaTime;
	std::chrono::high_resolution_clock::time_point lastTime;
	std::chrono::high_resolution_clock::time_point startTime;
};
