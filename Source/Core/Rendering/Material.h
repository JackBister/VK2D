#pragma once

#include "Core/Deserializable.h"

struct Accessor;
struct Program;

struct Material : Deserializable
{
	Material() noexcept;
	Material(ResourceManager *, const std::string&) noexcept;
	Deserializable * Deserialize(ResourceManager * resourceManager, const std::string & str, Allocator & alloc = Allocator::default_allocator) const final override;

	std::shared_ptr<Accessor> GetAccessor() const noexcept;
	std::shared_ptr<Program> GetProgram() const noexcept;

private:
	std::shared_ptr<Accessor> accessor;
	std::shared_ptr<Program> program;
};

DESERIALIZABLE_IMPL(Material)
