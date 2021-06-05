#include "ComponentCreator.h"

#include "Logging/Logger.h"
#include "Templates/TemplateRenderer.h"
#include "Util/WriteToFile.h"

static auto const logger = Logger::Create("ComponentCreator");

bool ComponentCreator::CreateComponentCode(std::filesystem::path directory, std::string projectName,
                                           std::string componentName, std::vector<ComponentProperty> properties)
{
    logger.Info("Generating component code in directory={} for component with name={}", directory, componentName);
    SerializedArray serializedProperties;
    for (auto const & p : properties) {
        serializedProperties.push_back(SerializedObject::Builder()
                                           .WithString("name", p.name)
                                           .WithString("type", SerializedValueTypeToString(p.type))
                                           .Build());
    }
    auto templateCtx = SerializedObject::Builder()
                           .WithString("projectName", projectName)
                           .WithString("componentName", componentName)
                           .WithArray("properties", serializedProperties)
                           .Build();

    {
        auto templateHeaderPath = templateDirectory / "Component.h.tpl";
        auto outputHeaderPath = directory / (componentName + ".h");

        auto renderedHeaderOpt = templateRenderer->RenderTemplate(templateHeaderPath, templateCtx);
        if (!renderedHeaderOpt.has_value()) {
            logger.Error("Failed to create component '{}': template rendering failed, there is likely logging above "
                         "indicating why",
                         componentName);
            return false;
        }
        logger.Info("Header file template rendered for component with name={}", componentName);
        bool writeSuccess = WriteToFile(outputHeaderPath, renderedHeaderOpt.value());
        if (!writeSuccess) {
            logger.Error("Failed to create component '{}': writing rendered template to file={} failed.",
                         componentName,
                         outputHeaderPath);
            return false;
        }
        logger.Info("Header file written to file={} for component with name={}", outputHeaderPath, componentName);
    }

    {
        auto templateCppPath = templateDirectory / "Component.cpp.tpl";
        auto outputCppPath = directory / (componentName + ".cpp");

        auto renderedHeaderOpt = templateRenderer->RenderTemplate(templateCppPath, templateCtx);
        if (!renderedHeaderOpt.has_value()) {
            logger.Error(
                "Failed to create component '{}': cpp template rendering failed, there is likely logging above "
                "indicating why",
                componentName);
            return false;
        }
        logger.Info("cpp file template rendered for component with name={}", componentName);
        bool writeSuccess = WriteToFile(outputCppPath, renderedHeaderOpt.value());
        if (!writeSuccess) {
            logger.Error("Failed to create component '{}': writing rendered template to cpp file={} failed.",
                         componentName,
                         outputCppPath);
            return false;
        }
        logger.Info("cpp file written to file={} for component with name={}", outputCppPath, componentName);
    }

    return true;
}
