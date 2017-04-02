#include "Core/Rendering/BufferView.h"

BufferView::BufferView()
{
}

BufferView::BufferView(Buffer * buffer, size_t byteOffset, size_t byteLength, Target target)
	: buffer(buffer), byteOffset(byteOffset), byteLength(byteLength), target(target)
{
}
