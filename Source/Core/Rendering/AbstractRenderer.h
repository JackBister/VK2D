#pragma once

#include <cstdint>

class IRenderer
{
public:
	virtual uint32_t GetSwapCount() = 0;
};
