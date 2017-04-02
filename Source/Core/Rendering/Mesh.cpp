#include "Core/Rendering/Mesh.h"

#include <glm/glm.hpp>
#include <json.hpp>

#include "Core/Rendering/Accessor.h"
#include "Core/Rendering/Program.h"
#include "Core/Maybe.h"

Mesh::Mesh() noexcept
{
}

Mesh::Mesh(ResourceManager * resMan, const std::string& name) noexcept
{
}

Deserializable * Mesh::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(Mesh));
	Mesh * ret = new (mem) Mesh();
	return ret;
}

Mesh::Primitive::Primitive() noexcept
{
}

Mesh::Primitive::Primitive(BufferView bv, std::shared_ptr<Material> mat, Mode m) noexcept : mode(m), buffer(bv), material(mat)
{
}

SubmittedMesh Mesh::Primitive::GetMeshSubmission() const noexcept
{
	return SubmittedMesh{
		material->GetAccessor()->GetRendererData(),
		buffer,
		material->GetProgram()->GetRendererData()
	};
}
