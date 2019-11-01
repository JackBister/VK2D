#pragma once

#include "FileSlurper.h"

class DefaultFileSlurper : public FileSlurper {
	virtual std::string slurpFile(std::string const& fileName) override;
};
