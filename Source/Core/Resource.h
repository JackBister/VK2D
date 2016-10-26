#pragma once

#include <string>
#include <type_traits>
#include <vector>

struct Resource
{
	std::string name;
	virtual ~Resource() {}
};

#define RESOURCE_HEADER(str) static_assert(std::is_base_of<Resource, str >::value, "[ERROR] Resource " #str " must be subclass of Resource!\n");\
							 static_assert(std::is_constructible< str , const std::string&>::value, "[ERROR] Resource " #str " must be default constructible\n");\
							 static_assert(std::is_constructible< str , const std::string&, const std::vector<char>&>::value, "[ERROR] Resource " #str " must be constructible with const std::vector<char>& as argument.\n");
