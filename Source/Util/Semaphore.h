#pragma once

#include <cstdint>
#include <memory>

class Semaphore
{
public:
    Semaphore();
    ~Semaphore();

    void Signal();
    void Wait();
    void WaitTimeout(uint32_t timeoutMs);

private:
    class Pimpl;

    std::unique_ptr<Pimpl> pimpl;
};
