#include "Core/Rendering/Buffer.h"

#include "Core/ResourceManager.h"

Buffer::Buffer(ResourceManager * resMan, size_t size, Type type) : size(size), type(type), usedBytes(0)
{
	resMan->PushRenderCommand(RenderCommand(RenderCommand::AddBufferParams(&rendererData, size, type)));
}
