#pragma once

#include "FileSlurper.h"

class DefaultFileSlurper : public FileSlurper
{
public:
    virtual std::string SlurpFile(std::string const & fileName) override;
};
