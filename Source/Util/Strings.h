#pragma once

#include <string>
#include <vector>

namespace Strings
{
bool ContainsWhitespace(std::string const &);
std::vector<std::string> Split(std::string const &);
std::string Trim(std::string);
}
