#include "GlslToSpirvShaderCompiler.h"

#include <cassert>
#include <filesystem>
#include <shaderc/shaderc.hpp>

shaderc_shader_kind GetShaderType(std::string const& fileExtension) {
    if (fileExtension == ".vert") {
        return shaderc_shader_kind::shaderc_vertex_shader;
    } else if (fileExtension == ".frag") {
        return shaderc_shader_kind::shaderc_fragment_shader;
    } else {
        assert(false);
        return shaderc_shader_kind::shaderc_vertex_shader;
    }
}

class DefaultIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
	explicit DefaultIncluder(std::shared_ptr<FileSlurper> fileSlurper) : fileSlurper(fileSlurper) {}

    shaderc_include_result *GetInclude(const char * requested_source,
                                               shaderc_include_type type,
                                               const char * requesting_source,
                                               size_t include_depth)
    override
    {
        std::filesystem::path requestingSourcePath(requesting_source);
        auto requestedPath = requestingSourcePath.remove_filename().append(requested_source);
        auto fileContent = fileSlurper->slurpFile(requestedPath.string());
		auto contentCopy = new char[fileContent.size() + 1];
		strcpy(contentCopy, fileContent.c_str());

        auto res = new shaderc_include_result();
		res->content = contentCopy;
        res->content_length = fileContent.size() - 1;
        res->source_name = requested_source;
		res->source_name_length = strlen(requested_source);
        res->user_data = nullptr;
        return res;
    }

    void ReleaseInclude(shaderc_include_result * data) override
    {
		delete data->content;
        delete data;
    }

private:
	std::shared_ptr<FileSlurper> fileSlurper;
};

SpirvCompilationResult GlslToSpirvShaderCompiler::CompileGlslFile(std::string const &fileName)
{
    std::filesystem::path filePath(fileName);
    auto fileContent = fileSlurper->slurpFile(fileName);
    auto shaderType = GetShaderType(filePath.extension().string());

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetIncluder(std::make_unique<DefaultIncluder>(fileSlurper));

    auto result = compiler.CompileGlslToSpv(fileContent.data(), fileContent.size() - 1, shaderType, fileName.c_str(), options);

	if (result.GetNumErrors() > 0) {
		return SpirvCompilationResult(
			std::vector<uint8_t>(),
			result.GetErrorMessage(),
			false);
	}

	// There is probably a better way of doing this...
	std::vector<uint32_t> compiledCode(result.begin(), result.end());
	std::vector<uint8_t> asUint8((uint8_t *)&compiledCode[0], (uint8_t *)&compiledCode[0] + compiledCode.size());

	return SpirvCompilationResult(
		asUint8,
		{},
		true);
}
