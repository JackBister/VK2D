#pragma once
#include <memory>

#include "Core/Deserializable.h"
#include "Core/Rendering/BufferView.h"
#include "Core/Rendering/Renderer.h"
#include "Core/Rendering/RendererData.h"

struct Accessor : Deserializable
{
	friend struct Renderer;
	
	enum class ComponentType
	{
		BYTE = 5120,
		UNSIGNED_BYTE = 5121,
		SHORT = 5122,
		UNSIGNED_SHORT = 5123,
		FLOAT = 5126
	};

	enum class Type
	{
		SCALAR,
		VEC2,
		VEC3,
		VEC4,
		MAT2,
		MAT3,
		MAT4
	};

	struct Attribute
	{
		size_t attribIndex;
		size_t size;
		ComponentType type;
		bool normalized;
		size_t relativeOffset;
	};

	Accessor() noexcept;
	Accessor(ResourceManager *, const std::string&) noexcept;

	Deserializable * Deserialize(ResourceManager * resourceManager, const std::string & str, Allocator & alloc = Allocator::default_allocator) const final override;
	AccessorRendererData GetRendererData() const noexcept;
private:
	std::vector<Attribute> attribs;
	AccessorRendererData rendererData;
};

DESERIALIZABLE_IMPL(Accessor)
