#pragma once
#include <memory>

#include "Core/Resource.h"
#include "Core/Rendering/Buffer.h"

struct ResourceManager;

struct BufferView
{
	friend class Accessor;
	enum class Target
	{
		ARRAY_BUFFER = 34962,
		ELEMENT_ARRAY_BUFFER = 34963
	};

	BufferView();
	BufferView(Buffer *, size_t byteOffset, size_t byteLength, Target);

private:
	Buffer * buffer;
	size_t byteOffset;
	size_t byteLength;
	Target target;
};
