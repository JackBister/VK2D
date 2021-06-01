#pragma once

#include <memory>

#include <SDL2/SDL.h>

class Semaphore
{
public:
	Semaphore();

	void Signal();
	void Wait();
private:
	struct SemDestructor
	{
		void operator()(SDL_sem * sem)
		{
			SDL_DestroySemaphore(sem);
		}
	};

	std::unique_ptr<SDL_sem, SemDestructor> sem_;
};
