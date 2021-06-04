#pragma once

#include "TemplateRenderer.h"

class FileSlurper;

class DefaultTemplateRenderer : public TemplateRenderer
{
public:
    DefaultTemplateRenderer(FileSlurper * fileSlurper) : fileSlurper(fileSlurper) {}

    std::optional<std::string> RenderTemplate(std::filesystem::path templatePath, SerializedObject ctx) override;

private:
    FileSlurper * fileSlurper;
};
