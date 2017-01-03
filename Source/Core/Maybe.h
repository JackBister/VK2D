#pragma once

#include <experimental/variant.hpp>

struct None {};

template <typename T>
using Maybe = std::experimental::variant<None, T>;
