#pragma once
#include <cstddef>

struct Allocator
{
	virtual void * Allocate(size_t size) = 0;
	virtual void Free(void * p) = 0;

	struct Default_Allocator;
	static Default_Allocator default_allocator;
};

struct Allocator::Default_Allocator : Allocator
{
	void * Allocate(size_t size) override;
	void Free(void * p) override;
};
