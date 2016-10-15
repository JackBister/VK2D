#pragma once
#include <unordered_map>

#include "Core/Allocator.h"

struct Deserializable
{
	std::string type;
	/*
		When Deserialize is called, a new object must be allocated using the provided allocator and the information contained in str
		must be deserialized into the new object. The new object is then returned.
	*/
	virtual Deserializable * Deserialize(const std::string& str, Allocator& alloc = Allocator::default_allocator) const = 0;

	static Deserializable * DeserializeString(const std::string& str, Allocator& alloc = Allocator::default_allocator);

	static std::unordered_map<std::string, const Deserializable *>& Map();
};

#define DESERIALIZABLE_IMPL(str) static const str * const _##str##Instantiate() \
							  { \
									if(Deserializable::Map()[#str] != nullptr) return static_cast< const str * const >(Deserializable::Map()[#str]); \
									const str * const ret = new str(); Deserializable::Map()[#str] = ret; return ret; \
							  } \
							  static const Deserializable * const _##str##StaticInst = _##str##Instantiate();
