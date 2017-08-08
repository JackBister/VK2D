#pragma once

#include <chrono>

class Time
{
public:
	void Frame();
	float get_delta_time();
	std::chrono::high_resolution_clock::time_point get_last_time();
	void Start(float timeScale = 1.f);

	float time_scale_;
private:
	float delta_time_;
	std::chrono::high_resolution_clock::time_point last_time_;
	std::chrono::high_resolution_clock::time_point start_time_;
};
