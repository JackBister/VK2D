#pragma once

#include <vector>

#include "Core/Deserializable.h"
#include "Core/Rendering/BufferView.h"
#include "Core/Rendering/Material.h"
#include "Core/Rendering/SubmittedMesh.h"

struct Mesh : Deserializable
{
	struct Primitive
	{
		enum class Mode
		{
			POINTS,
			LINES,
			LINE_LOOP,
			LINE_STRIP,
			TRIANGLES,
			TRIANGLE_STRIP,
			TRIANGLE_FAN
		};
		Mode mode;

		Primitive() noexcept;
		Primitive(BufferView, std::shared_ptr<Material>, Mode) noexcept;

		SubmittedMesh GetMeshSubmission() const noexcept;
	private:
		BufferView buffer;
		std::shared_ptr<Material> material;
	};

	Mesh() noexcept;
	Mesh(ResourceManager *, const std::string&) noexcept;

	Deserializable * Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc = Allocator::default_allocator) const final override;

	std::vector<Primitive> primitives;
};

DESERIALIZABLE_IMPL(Mesh)
