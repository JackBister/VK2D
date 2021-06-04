#include "ComponentCreator.h"

#include "Logging/Logger.h"
#include "Templates/TemplateRenderer.h"
#include "Util/WriteToFile.h"

static auto const logger = Logger::Create("ComponentCreator");

bool ComponentCreator::CreateComponentCode(std::filesystem::path directory, std::string componentName,
                                           std::vector<ComponentProperty> properties)
{
    logger->Infof("Generating component code in directory=%ls for component with name=%s",
                  directory.c_str(),
                  componentName.c_str());
    SerializedArray serializedProperties;
    for (auto const & p : properties) {
        serializedProperties.push_back(SerializedObject::Builder()
                                           .WithString("name", p.name)
                                           .WithString("type", SerializedValueTypeToString(p.type))
                                           .Build());
    }
    auto templateCtx = SerializedObject::Builder()
                           .WithString("componentName", componentName)
                           .WithArray("properties", serializedProperties)
                           .Build();

    {
        auto templateHeaderPath = templateDirectory / "Component.h.tpl";
        auto outputHeaderPath = directory / (componentName + ".h");

        auto renderedHeaderOpt = templateRenderer->RenderTemplate(templateHeaderPath, templateCtx);
        if (!renderedHeaderOpt.has_value()) {
            logger->Errorf("Failed to create component '%s': template rendering failed, there is likely logging above "
                           "indicating why",
                           componentName.c_str());
            return false;
        }
        logger->Infof("Header file template rendered for component with name=%s", componentName.c_str());
        bool writeSuccess = WriteToFile(outputHeaderPath, renderedHeaderOpt.value());
        if (!writeSuccess) {
            logger->Errorf("Failed to create component '%s': writing rendered template to file=%ls failed.",
                           componentName.c_str(),
                           outputHeaderPath.c_str());
            return false;
        }
        logger->Infof("Header file written to file=%ls for component with name=%s",
                      outputHeaderPath.c_str(),
                      componentName.c_str());
    }

    {
        auto templateCppPath = templateDirectory / "Component.cpp.tpl";
        auto outputCppPath = directory / (componentName + ".cpp");

        auto renderedHeaderOpt = templateRenderer->RenderTemplate(templateCppPath, templateCtx);
        if (!renderedHeaderOpt.has_value()) {
            logger->Errorf(
                "Failed to create component '%s': cpp template rendering failed, there is likely logging above "
                "indicating why",
                componentName.c_str());
            return false;
        }
        logger->Infof("cpp file template rendered for component with name=%s", componentName.c_str());
        bool writeSuccess = WriteToFile(outputCppPath, renderedHeaderOpt.value());
        if (!writeSuccess) {
            logger->Errorf("Failed to create component '%s': writing rendered template to cpp file=%ls failed.",
                           componentName.c_str(),
                           outputCppPath.c_str());
            return false;
        }
        logger->Infof(
            "cpp file written to file=%ls for component with name=%s", outputCppPath.c_str(), componentName.c_str());
    }

    return true;
}
