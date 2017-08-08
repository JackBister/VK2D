#include "Core/dtime.h"

using std::chrono::high_resolution_clock;

void Time::Start(float ts)
{
	time_scale_ = ts;
	start_time_ = high_resolution_clock::now();
	last_time_ = start_time_;
}

void Time::Frame()
{
	high_resolution_clock::time_point currTime = high_resolution_clock::now();
	delta_time_ = time_scale_ * std::chrono::duration<float>(currTime - last_time_).count();
	last_time_ = currTime;
}

float Time::get_delta_time()
{
	return delta_time_;
}

std::chrono::high_resolution_clock::time_point Time::get_last_time()
{
	return last_time_;
}
