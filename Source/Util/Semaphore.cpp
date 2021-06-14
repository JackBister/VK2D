#include "Semaphore.h"

#include <ThirdParty/optick/src/optick.h>
#include <thirdparty/SDL2/include/SDL.h>

class Semaphore::Pimpl
{
public:
    struct SemDestructor {
        void operator()(SDL_sem * sem) { SDL_DestroySemaphore(sem); }
    };

    Pimpl() : sem(std::unique_ptr<SDL_sem, SemDestructor>(SDL_CreateSemaphore(0))) {}

    std::unique_ptr<SDL_sem, SemDestructor> sem;
};

Semaphore::Semaphore() : pimpl(std::make_unique<Pimpl>())
{
    OPTICK_EVENT();
}

Semaphore::~Semaphore() = default;

void Semaphore::Signal()
{
    OPTICK_EVENT();
    SDL_SemPost(pimpl->sem.get());
}

void Semaphore::Wait()
{
    SDL_SemWait(pimpl->sem.get());
}
