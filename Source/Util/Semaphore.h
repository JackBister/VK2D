#pragma once

#include <memory>

class Semaphore
{
public:
    Semaphore();
    ~Semaphore();

    void Signal();
    void Wait();

private:
    class Pimpl;

    std::unique_ptr<Pimpl> pimpl;
};
