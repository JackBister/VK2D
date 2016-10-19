#pragma once

struct Resource
{
	std::string name;
	virtual ~Resource() {}
};

#define RESOURCE_HEADER(str) static_assert(std::is_base_of<Resource, str >::value, "[ERROR] Resource " #str " must be subclass of Resource!\n");\
							 static_assert(std::is_constructible< str , const std::string&>::value, "[ERROR] Resource " #str " must be default constructible\n");\
							 static_assert(std::is_constructible< str , const std::string&, std::istream&>::value, "[ERROR] Resource " #str " must be constructible with istream& as argument.\n");
