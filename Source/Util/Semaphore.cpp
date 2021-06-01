#include "Semaphore.h"

#include <optick/optick.h>

Semaphore::Semaphore()
{
    OPTICK_EVENT();
    sem_ = std::unique_ptr<SDL_semaphore, SemDestructor>(SDL_CreateSemaphore(0));
}

void Semaphore::Signal()
{
    OPTICK_EVENT();
    SDL_SemPost(sem_.get());
}

void Semaphore::Wait()
{
    SDL_SemWait(sem_.get());
}
