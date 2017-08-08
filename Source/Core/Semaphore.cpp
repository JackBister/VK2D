#include "Semaphore.h"

Semaphore::Semaphore() : sem_(SDL_CreateSemaphore(0))
{
}

void Semaphore::Signal()
{
	SDL_SemPost(sem_.get());
}

void Semaphore::Wait()
{
	SDL_SemWait(sem_.get());
}
