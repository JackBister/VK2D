#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "Serialization/SerializedValue.h"

class TemplateRenderer
{
public:
    virtual std::optional<std::string> RenderTemplate(std::filesystem::path templatePath, SerializedObject ctx) = 0;

private:
};
