#include "DefaultFileSlurper.h"

#include <cassert>
#include <cstdio>
#include <vector>

std::string DefaultFileSlurper::slurpFile(std::string const& fileName)
{
	FILE* f = fopen(fileName.c_str(), "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		fseek(f, 0, SEEK_SET);
		std::vector<char> buf(length + 1);
		fread(&buf[0], 1, length, f);
		return std::string(buf.begin(), buf.end());
	} else {
		printf("[ERROR] DefaultFileSlurper failed to open file %s\n", fileName.c_str());
		assert(false);
		return "";
	}
}
