#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "Core/Util/FileSlurper.h"

class SpirvCompilationResult
{
public:
    SpirvCompilationResult(std::vector<uint32_t> compiledCode, std::optional<std::string> errorMessage,
                           bool isSuccessful)
        : compiledCode(compiledCode), errorMessage(errorMessage), success(isSuccessful)
    {
    }

    std::vector<uint32_t> GetCompiledCode() { return compiledCode; }
    std::optional<std::string> GetErrorMessage() { return errorMessage; }
    bool IsSuccessful() { return success; }

private:
    std::vector<uint32_t> compiledCode;
    std::optional<std::string> errorMessage;
    bool success;
};

class GlslToSpirvShaderCompiler
{
public:
    explicit GlslToSpirvShaderCompiler(std::shared_ptr<FileSlurper> fileSlurper) : fileSlurper(fileSlurper) {}

    SpirvCompilationResult CompileGlslFile(std::string const & fileName);

private:
    std::shared_ptr<FileSlurper> fileSlurper;
};
