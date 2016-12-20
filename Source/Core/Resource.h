#pragma once

#include <string>
#include <type_traits>
#include <vector>

struct ResourceManager;

struct Resource
{
	std::string name;
	virtual ~Resource() {}
};

template <typename T>
struct is_resource
{
	static const bool value = std::is_base_of<Resource, T>::value && std::is_constructible<T, ResourceManager *, const std::string&>::value && std::is_constructible<T, ResourceManager *, const std::string&, const std::vector<char>&>::value;
};

#define RESOURCE_HEADER(str) static_assert(std::is_base_of<Resource, str >::value, "[ERROR] Resource " #str " must be subclass of Resource!\n");\
							 static_assert(std::is_constructible< str , ResourceManager *, const std::string&>::value, "[ERROR] Resource " #str " must be default constructible\n");\
							 static_assert(std::is_constructible< str , ResourceManager *, const std::string&, const std::vector<char>&>::value, "[ERROR] Resource " #str " must be constructible with const std::vector<char>& as argument.\n");
