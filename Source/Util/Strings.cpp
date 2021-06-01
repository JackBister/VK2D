#include "Strings.h"

#include <iterator>
#include <sstream>

const std::string WHITESPACE = " \n\r\t\f\v";

namespace Strings
{
bool ContainsWhitespace(std::string const & str)
{
    return str.find(WHITESPACE) != std::string::npos;
}

std::vector<std::string> Split(std::string const & str)
{
    std::istringstream iss(str);
    // Will std::string ever have a split function? Does anyone enjoy code like this?
    return std::vector(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
}

std::string Trim(std::string str)
{
    size_t start = str.find_first_not_of(WHITESPACE);
    str = (start == std::string::npos) ? "" : str.substr(start);

    size_t end = str.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}
}