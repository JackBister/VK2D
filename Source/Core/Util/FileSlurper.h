#pragma once

#include <string>

class FileSlurper {
public:
	virtual std::string slurpFile(std::string const& fileName) = 0;
};