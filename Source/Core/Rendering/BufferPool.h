#pragma once

#include <vector>

#include "Core/Rendering/Buffer.h"
#include "Core/Rendering/BufferView.h"

struct BufferPool
{

	BufferView RequestBuffer(size_t requiredSize, Buffer::Type requiredType, BufferView::Target target);

	BufferPool(ResourceManager *, size_t allocSize);

private:
	size_t allocSize;
	std::vector<Buffer> pool;
	ResourceManager * resMan;
};
