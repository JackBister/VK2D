#pragma once

#include <optional>
#include <string>
#include <vector>

namespace Strings
{
bool ContainsWhitespace(std::string const &);
std::vector<std::string> Split(std::string const &);
std::optional<double> Strtod(std::string const &);
std::string Trim(std::string);
}
