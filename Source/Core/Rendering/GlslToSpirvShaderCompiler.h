#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "./Context/RenderContext.h"
#include "Core/Util/FileSlurper.h"

class SpirvCompilationResult {
public:
	SpirvCompilationResult(std::vector<uint8_t> compiledCode, std::optional<std::string> errorMessage, bool isSuccessful)
		: compiledCode(compiledCode), errorMessage(errorMessage), success(isSuccessful) {}

	std::vector<uint8_t> getCompiledCode() { return compiledCode; }
	std::optional<std::string> getErrorMessage() { return errorMessage; }
	bool isSuccessful() { return success; }
private:
	std::vector<uint8_t> compiledCode;
	std::optional<std::string> errorMessage;
	bool success;
};

class GlslToSpirvShaderCompiler {
public:
	explicit GlslToSpirvShaderCompiler(std::shared_ptr<FileSlurper> fileSlurper) : fileSlurper(fileSlurper) {}

	SpirvCompilationResult CompileGlslFile(std::string const& fileName);

private:
	std::shared_ptr<FileSlurper> fileSlurper;
};

