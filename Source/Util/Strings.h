#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Strings
{
bool ContainsWhitespace(std::string const &);
std::vector<std::string> Split(std::string const &, char c = ' ');
std::optional<double> Strtod(std::string const &);
std::string Trim(std::string);
std::string_view TrimView(std::string_view);
}
