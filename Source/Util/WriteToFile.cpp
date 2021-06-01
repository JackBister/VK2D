#include "WriteToFile.h"

#include <fstream>

bool WriteToFile(std::filesystem::path path, std::string const & str)
{
    auto outStream = std::ofstream(path);
    outStream << str;
    return true;
}