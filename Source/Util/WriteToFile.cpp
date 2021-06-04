#include "WriteToFile.h"

#include <fstream>

bool WriteToFile(std::filesystem::path path, std::string str)
{
    auto outStream = std::ofstream(path, std::ios::binary);
    outStream << str;
    return true;
}