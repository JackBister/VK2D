#pragma once

#include <string>

class FileSlurper
{
public:
    virtual std::string SlurpFile(std::string const & fileName) = 0;
};