#pragma once

#include <functional>

#include "DllExport.h"

class EAPI HashedString {
	friend struct std::hash<HashedString>;
public:
	template <unsigned N>
	constexpr HashedString(const char(&arr)[N]) : hash(0)
	{
		for (unsigned i = 0; i < N; ++i) {
			hash += arr[i] * 31 ^ (N - i + 1);
		}
	}

	/*
		You generally want to avoid this one. I would like to add a good warning for this at some point but it's going to be a nag if done wrong.
	*/
	HashedString(std::string const& arr) : hash(0)
	{
		auto N = arr.size();
		for (unsigned i = 0; i < N; ++i) {
			hash += arr[i] * 31 ^ ((int)N - i + 1);
		}
	}

	constexpr bool operator==(const HashedString rhs) const
	{
		return hash == rhs.hash;
	}

	constexpr bool operator!=(const HashedString rhs) const
	{
		return hash != rhs.hash;
	}

private:
	int hash;
};

namespace std
{
	template<>
	struct hash<HashedString>
	{
		size_t operator()(HashedString const s) const
		{
			return s.hash;
		}
	};
}
