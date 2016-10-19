#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "Core/Allocator.h"
#include "Core/Resource.h"

#include <cstdio>

template <typename T>
struct ResourceReference;

struct ResourceManager
{
	template <typename T>
	friend class ResourceReference;

	ResourceManager(Allocator& a = Allocator::default_allocator);

	template<typename T>
	typename std::enable_if<std::is_base_of<Resource, T>::value && std::is_constructible<T, const std::string&>::value && std::is_constructible<T, const std::string&, std::istream&>::value, ResourceReference<T>>::type
	LoadResource(const std::string& fileName);

private:
	std::unordered_map<std::string, Resource *> cache;
	Allocator& allocator;
};

template <typename T>
struct ResourceReference
{
	friend class ResourceManager;

	T& operator*();
	T * operator->();

private:
	std::string name;
	ResourceManager * owner;
	ResourceReference<T>(const std::string& name, ResourceManager * owner);
};

template<typename T>
T& ResourceReference<T>::operator*()
{
	return static_cast<T&>(*(owner->cache[name]));
}

template<typename T>
T * ResourceReference<T>::operator->()
{
	return static_cast<T *>(owner->cache[name]);
}

template<typename T>
ResourceReference<T>::ResourceReference(const std::string& name, ResourceManager * owner) : name(name), owner(owner)
{
}

template<typename T>
typename std::enable_if<std::is_base_of<Resource, T>::value && std::is_constructible<T, const std::string&>::value && std::is_constructible<T, const std::string&, std::istream&>::value, ResourceReference<T>>::type
ResourceManager::LoadResource(const std::string& fileName)
{
	if (cache.find(fileName) != cache.end()) {
		printf("found\n");
		return ResourceReference<T>(fileName, this);
	}
	boost::filesystem::path filePath(fileName);
	auto status = boost::filesystem::status(filePath);
	Resource * mem = (Resource *)allocator.Allocate(sizeof(T));
	if (status.type() != boost::filesystem::regular_file) {
		cache[fileName] = new (mem) T(fileName);
	} else {
		boost::filesystem::ifstream is(filePath);
		cache[fileName] = new (mem) T(fileName, is);
		is.close();
	}
	return ResourceReference<T>(fileName, this);
}
