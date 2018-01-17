#pragma once

#include "DllExport.h"

class EAPI HashedString {
public:
	template <unsigned N>
	constexpr HashedString(const char(&arr)[N]) : hash(0) {
		for (unsigned i = 0; i < N; ++i) {
			hash += arr[i] * 31 ^ (N - i + 1);
		}
	}

	constexpr bool operator==(const HashedString rhs) const {
		return hash == rhs.hash;
	}

	constexpr bool operator!=(const HashedString rhs) const {
		return hash != rhs.hash;
	}

private:
	int hash;
};
