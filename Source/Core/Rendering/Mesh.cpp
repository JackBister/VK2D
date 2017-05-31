#include "Core/Rendering/Mesh.h"

Mesh::Mesh(BufferHandle * vbo, BufferHandle * ebo) : vbo(vbo), ebo(ebo), type(ebo != nullptr ? Type::WITH_EBO : Type::WITHOUT_EBO)
{
	assert(vbo != nullptr);
}

Mesh::Type Mesh::GetType()
{
	return type;
}

BufferHandle * Mesh::GetEBO()
{
	assert(ebo != nullptr);
	return ebo;
}

BufferHandle * Mesh::GetVBO()
{
	assert(vbo != nullptr);
	return vbo;
}
