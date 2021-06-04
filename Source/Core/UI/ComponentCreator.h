#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Serialization/SerializedValueType.h"

class TemplateRenderer;

struct ComponentProperty {
    SerializedValueType const type;
    std::string const name;
};

class ComponentCreator
{

public:
    ComponentCreator(TemplateRenderer * templateRenderer,
                     std::filesystem::path templateDirectory = "templates/NewComponent")
        : templateRenderer(templateRenderer), templateDirectory(templateDirectory)
    {
    }

    bool CreateComponentCode(std::filesystem::path directory, std::string componentName,
                             std::vector<ComponentProperty> properties);

private:
    TemplateRenderer * templateRenderer;

    std::filesystem::path templateDirectory;
};
