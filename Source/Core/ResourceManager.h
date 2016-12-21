#pragma once
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "Core/Allocator.h"
#include "Core/Resource.h"

struct ResourceManager
{
	ResourceManager(Allocator& a = Allocator::default_allocator);

	template <typename T>
	typename std::enable_if<is_resource<T>::value, void>::type
	AddResourceRefCounted(const std::string& name, const std::weak_ptr<Resource> res);

	template<typename T>
	typename std::enable_if<is_resource<T>::value, std::shared_ptr<T>>::type
	LoadResourceRefCounted(const std::string& fileName);

private:
	std::unordered_map<std::string, std::weak_ptr<Resource>> rcCache;
	Allocator& allocator;
};

template <typename T>
typename std::enable_if<is_resource<T>::value, void>::type
ResourceManager::AddResourceRefCounted(const std::string& name, const std::weak_ptr<Resource> res)
{
	//TODO: Check if exists? Pointless?
	rcCache[name] = res;
}

template<typename T>
typename std::enable_if<is_resource<T>::value, std::shared_ptr<T>>::type
ResourceManager::LoadResourceRefCounted(const std::string& fileName)
{
	if (rcCache.find(fileName) != rcCache.end() && !rcCache[fileName].expired()) {
		auto ret = std::static_pointer_cast<T>(rcCache[fileName].lock());
		if (ret) {
			return ret;
		}
	}
	boost::filesystem::path filePath(fileName);
	auto status = boost::filesystem::status(filePath);
	Resource * mem = (Resource *)allocator.Allocate(sizeof(T));
	if (status.type() != boost::filesystem::regular_file) {
		auto ret = std::shared_ptr<T>(new (mem) T(this, fileName));
		rcCache[fileName] = std::weak_ptr<Resource>(ret);
		return ret;
	} else {
		//For some awful reason this wont work for binary files like PNG. Otherwise I think it's a prettier solution.
#if 0
		boost::filesystem::ifstream is(fileName, std::ios::in | std::ios::binary);
		auto ret = std::shared_ptr<T>(new (mem) T(fileName, is));
#endif
		FILE * f = fopen(fileName.c_str(), "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			size_t length = ftell(f);
			fseek(f, 0, SEEK_SET);
			std::vector<char> buf(length + 1);
			fread(&buf[0], 1, length, f);
			auto ret = std::shared_ptr<T>(new (mem) T(this, fileName, buf));
			rcCache[fileName] = std::weak_ptr<Resource>(ret);
			fclose(f);
			return ret;
		} else {
			auto ret = std::shared_ptr<T>(new (mem) T(this, fileName));
			rcCache[fileName] = std::weak_ptr<Resource>(ret);
			return ret;
		}
	}
}
