#include "Core/Rendering/BufferPool.h"

//TODO: not thread safe
BufferView BufferPool::RequestBuffer(size_t requiredSize, Buffer::Type requiredType, BufferView::Target target)
{
	for (auto& buffer : pool) {
		if (buffer.size - buffer.usedBytes >= requiredSize && buffer.type == requiredType) {
			buffer.usedBytes += requiredSize;
			return BufferView(&buffer, buffer.usedBytes, requiredSize, target);
		}
	}
	if (requiredSize <= allocSize) {
		pool.emplace_back(resMan, allocSize, requiredType);
	} else {
		pool.emplace_back(resMan, requiredSize, requiredType);
	}
	return BufferView(&pool.back(), 0, requiredSize, target);
}

BufferPool::BufferPool(ResourceManager * resMan, size_t allocSize) : allocSize(allocSize), resMan(resMan)
{
}
