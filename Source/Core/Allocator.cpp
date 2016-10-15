#include "Allocator.h"

#include <cstdlib>

Allocator::Default_Allocator Allocator::default_allocator;

void * Allocator::Default_Allocator::Allocate(size_t size)
{
	return malloc(size);
}

void Allocator::Default_Allocator::Free(void * p)
{
	free(p);
}
