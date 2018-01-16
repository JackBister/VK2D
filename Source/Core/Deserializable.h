#pragma once
#include <unordered_map>

#include "Core/Allocator.h"
#include "Core/DllExport.h"
#include "Core/ResourceManager.h"

class EAPI Deserializable
{
public:
	/*
		When Deserialize is called, a new object must be allocated using the provided allocator and the information contained in str
		must be deserialized into the new object. The new object is then returned.
	*/
	virtual Deserializable * Deserialize(ResourceManager * resourceManager, std::string const& str) const = 0;

	/*
		Serializes an object into a format suitable for storing in a scene file (currently JSON)
	*/
	virtual std::string Serialize() const = 0;

	/*
		Deserializes a string of unknown type.
	*/
	static Deserializable * DeserializeString(ResourceManager * resourceManager, std::string const& str);

	static std::unordered_map<std::string, Deserializable const *>& Map();

	std::string type;
};

#ifdef VK2D_DLL
#define DESERIALIZABLE_IMPL(str)
#elif defined(VK2D_LIB)
#define DESERIALIZABLE_IMPL(str) static str const * _##str##Instantiate() \
							  { \
									if(Deserializable::Map()[#str] != nullptr) return static_cast<str const *>(Deserializable::Map()[#str]); \
									str const * ret = new str(); Deserializable::Map()[#str] = ret; return ret; \
							  } \
							  extern "C" Deserializable const * _##str##StaticInst = _##str##Instantiate(); 
#else
#define DESERIALIZABLE_IMPL(str) static str const * _##str##Instantiate() \
							  { \
									if(Deserializable::Map()[#str] != nullptr) return static_cast<str const *>(Deserializable::Map()[#str]); \
									str const * ret = new str(); Deserializable::Map()[#str] = ret; return ret; \
							  } \
							  static Deserializable const * _##str##StaticInst = _##str##Instantiate();
#endif
