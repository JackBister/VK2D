#pragma once
#include <unordered_map>

#include "Core/Allocator.h"
#include "Core/ResourceManager.h"

class Deserializable
{
public:
	/*
		When Deserialize is called, a new object must be allocated using the provided allocator and the information contained in str
		must be deserialized into the new object. The new object is then returned.
	*/
	virtual Deserializable * Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc = Allocator::default_allocator) const = 0;

	/*
		Deserializes a string of unknown type.
	*/
	static Deserializable * DeserializeString(ResourceManager * resourceManager, std::string const& str, Allocator& alloc = Allocator::default_allocator);

	static std::unordered_map<std::string, Deserializable const *>& Map();

	std::string type;
};

#define DESERIALIZABLE_IMPL(str) static str const * _##str##Instantiate() \
							  { \
									if(Deserializable::Map()[#str] != nullptr) return static_cast<str const *>(Deserializable::Map()[#str]); \
									str const * ret = new str(); Deserializable::Map()[#str] = ret; return ret; \
							  } \
							  static Deserializable const * _##str##StaticInst = _##str##Instantiate();
