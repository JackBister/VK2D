#include "Semaphore.h"

Semaphore::Semaphore() : sem(SDL_CreateSemaphore(0))
{
}

void Semaphore::Signal()
{
	SDL_SemPost(sem.get());
}

void Semaphore::Wait()
{
	SDL_SemWait(sem.get());
}
