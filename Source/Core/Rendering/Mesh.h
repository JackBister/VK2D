#pragma once

#include <vector>

#include "Core/Deserializable.h"
#include "Core/Rendering/BufferView.h"
#include "Core/Rendering/Material.h"
#include "Core/Rendering/SubmittedMesh.h"

struct Mesh
{
	enum class Type
	{
		WITH_EBO,
		WITHOUT_EBO
	};

	Mesh(BufferHandle * vbo, BufferHandle * ebo = nullptr);

	Type GetType();
	BufferHandle * GetEBO();
	BufferHandle * GetVBO();

private:
	BufferHandle * ebo;
	BufferHandle * vbo;

	Type type;
};
