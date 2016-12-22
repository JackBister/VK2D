#pragma once
#include <unordered_map>

#include "Core/Allocator.h"
#include "Core/ResourceManager.h"

struct Deserializable
{
	std::string type;
	/*
		When Deserialize is called, a new object must be allocated using the provided allocator and the information contained in str
		must be deserialized into the new object. The new object is then returned.
	*/
	virtual Deserializable * Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc = Allocator::default_allocator) const = 0;

	/*
		Deserializes a string of unknown type.
	*/
	static Deserializable * DeserializeString(ResourceManager * resourceManager, const std::string& str, Allocator& alloc = Allocator::default_allocator);

	static std::unordered_map<std::string, const Deserializable *>& Map();
};

#define DESERIALIZABLE_IMPL(str) static const str * _##str##Instantiate() \
							  { \
									if(Deserializable::Map()[#str] != nullptr) return static_cast<const str *>(Deserializable::Map()[#str]); \
									const str * ret = new str(); Deserializable::Map()[#str] = ret; return ret; \
							  } \
							  static const Deserializable * const _##str##StaticInst = _##str##Instantiate();
