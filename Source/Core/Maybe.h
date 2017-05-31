#pragma once

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif

struct None {};

template <typename T>
using Maybe = std::variant<None, T>;
