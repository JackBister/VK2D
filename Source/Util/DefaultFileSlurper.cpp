#include "DefaultFileSlurper.h"

#include <cassert>
#include <cstdio>
#include <vector>

#include "Logging/Logger.h"

static const auto logger = Logger::Create("DefaultFileSlurper");

std::string DefaultFileSlurper::SlurpFile(std::string const & fileName)
{
    FILE * f = fopen(fileName.c_str(), "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t length = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (length == 0) {
            return "";
        }
        std::vector<char> buf(length);
        fread(&buf[0], 1, length, f);
        return std::string(buf.begin(), buf.end());
    } else {
        logger.Error("Failed to open fileName={}, will return empty string", fileName);
        assert(false);
        return "";
    }
}
