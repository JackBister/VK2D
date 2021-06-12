#include "ProjectCreator.h"

#include <array>

#include "ComponentCreator.h"
#include "Logging/Logger.h"
#include "Serialization/DeserializationContext.h"
#include "Serialization/SchemaValidator.h"
#include "Serialization/Serializer.h"
#include "Templates/TemplateRenderer.h"
#include "Util/FileSlurper.h"
#include "Util/WriteToFile.h"

static auto const logger = Logger::Create("ProjectCreator");

static std::array<std::string, 8> PROJECT_TEMPLATE_FILES{".gitignore",
                                                         ".vscode/c_cpp_properties.json",
                                                         ".vscode/tasks.json",
                                                         "CMakeLists.txt.tpl",
                                                         "main.scene.tpl",
                                                         "CMake/CopyAndRenameDll_booter.cmake.tpl",
                                                         "CMake/CopyAndRenameDll.cmake.tpl",
                                                         "Scripts/main.cpp.tpl"};

SerializedObjectSchema PARTIAL_PROJECT_SCHEMA = SerializedObjectSchema(
    "PartialProject", {SerializedPropertySchema::Required("name", SerializedValueType::STRING),
                       SerializedPropertySchema::RequiredObject("startingGravity", "Vec3"),
                       SerializedPropertySchema::RequiredObjectArray("defaultKeybindings", "Keybinding")});

class PartialProjectDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() override { return PARTIAL_PROJECT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) override
    {
        logger.Error("STUB: PartialProjectDeserializer::Deserialize");
        return nullptr;
    }
};

DESERIALIZABLE_IMPL(PartialProject, new PartialProjectDeserializer())

bool ProjectCreator::CreateProject(std::filesystem::path projectPath, SerializedObject partialProject)
{
    auto projectParentPath = projectPath.parent_path();
    std::filesystem::create_directories(projectParentPath);

    auto validationResult = SchemaValidator::Validate(PARTIAL_PROJECT_SCHEMA, partialProject);
    if (!validationResult.isValid) {
        logger.Error("Failed to create project: given object does not match the PartialProject schema. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger.Error("\t{}: {}", err.first, err.second);
        }
        return false;
    }

    SerializedArray dlls;
    dlls.push_back("build/bin/" + partialProject.GetString("name").value() + ".dll");
    auto projectObj =
        partialProject.ToBuilder().WithArray("dlls", dlls).WithString("startingScene", "./main.scene").Build();
    DeserializationContext ctx{.workingDirectory = projectParentPath};
    auto projectOpt = Project::Deserialize(&ctx, projectObj);
    if (!projectOpt.has_value()) {
        logger.Error("Failed to create project: failed to deserialize project after adding DLL and startingScene");
        return false;
    }
    auto project = projectOpt.value();

    bool writeProjectSuccess = WriteProjectFile(projectPath, project);
    if (!writeProjectSuccess) {
        return false;
    }

    bool writeComponentSuccess = WriteComponentFiles(projectParentPath, project.GetName());
    if (!writeComponentSuccess) {
        return false;
    }

    auto templateCtx = SerializedObject::Builder().WithString("projectName", project.GetName()).Build();

    for (auto const & templateFileName : PROJECT_TEMPLATE_FILES) {
        bool templateSuccess = WriteTemplateFile(templateFileName, projectParentPath, templateCtx);
    }

    bool copyLibrarySuccess = CopyLibraryFiles(projectParentPath);
    if (!copyLibrarySuccess) {
        return false;
    }

    auto renderedBuildPath = projectParentPath / "build";
    bool createBuildSuccess = std::filesystem::create_directories(renderedBuildPath);
    if (!createBuildSuccess) {
        logger.Error("Failed to create build directory at path={}", renderedBuildPath);
        return false;
    }

    return true;
}

bool ProjectCreator::WriteProjectFile(std::filesystem::path projectPath, Project p)
{
    logger.Info("Writing project file to path={} for project with name={}", projectPath, p.GetName());
    auto serializedProject = p.Serialize();
    auto serializedProjectString = serializer->Serialize(serializedProject, {.prettyPrint = true});

    bool projectWriteSuccess = WriteToFile(projectPath, serializedProjectString);
    if (!projectWriteSuccess) {
        logger.Error("Failed to write project with name={} to path={}", p.GetName(), projectPath);
        return false;
    }
    return true;
}

bool ProjectCreator::WriteComponentFiles(std::filesystem::path projectParentPath, std::string projectName)
{
    auto scriptPath = projectParentPath / "Scripts";
    logger.Info("Writing component files to path={}", scriptPath);
    std::filesystem::create_directories(scriptPath);
    bool componentSuccess = componentCreator->CreateComponentCode(
        scriptPath, projectName, "MyComponent", {{.type = SerializedValueType::DOUBLE, .name = "myProperty"}});
    if (!componentSuccess) {
        logger.Error("Failed to generate template component at path={}", projectParentPath);
        return false;
    }
    return true;
}

bool ProjectCreator::WriteTemplateFile(std::string templateFileName, std::filesystem::path projectParentPath,
                                       SerializedObject & templateCtx)
{
    auto templatePath = templateDirectory / templateFileName;
    logger.Info("Rendering template from path={} to project at path=%{}", templatePath, projectParentPath);
    if (templateFileName.ends_with(".tpl")) {
        std::optional<std::string> renderedOpt = templateRenderer->RenderTemplate(templatePath, templateCtx);
        if (!renderedOpt.has_value()) {
            logger.Error("Failed to render template with path={}", templatePath);
            return false;
        }

        std::string renderedFileName = templateFileName;
        size_t tplIdx = renderedFileName.find(".tpl");
        if (tplIdx != templateFileName.npos) {
            renderedFileName = renderedFileName.substr(0, tplIdx);
        }

        auto renderedPath = projectParentPath / renderedFileName;
        std::filesystem::create_directories(renderedPath.parent_path());
        bool writeSuccess = WriteToFile(renderedPath, renderedOpt.value());
        if (!writeSuccess) {
            logger.Error("Failed to write rendered template to path={}", renderedPath);
            return false;
        }
    } else {
        auto renderedPath = projectParentPath / templateFileName;
        std::filesystem::create_directories(renderedPath.parent_path());
        std::error_code copyError;
        bool copySuccess = std::filesystem::copy_file(
            templatePath, renderedPath, std::filesystem::copy_options::overwrite_existing, copyError);
        if (copyError) {
            logger.Error("Failed to copy template file from templatePath={} to renderedPath={}, error={}",
                         templatePath,
                         renderedPath,
                         copyError.message());
            return false;
        }
        if (!copySuccess) {
            logger.Error(
                "Failed to copy template file from templatePath={} to renderedPath={}", templatePath, renderedPath);
        }
        return false;
    }
    return true;
}

bool ProjectCreator::CopyLibraryFiles(std::filesystem::path projectParentPath)
{
    auto templateIncludePath = templateDirectory / "Include";
    auto renderedIncludePath = projectParentPath / "Include";
    logger.Info(
        "Copying include files from templatePath={} to renderedPath={}", templateIncludePath, renderedIncludePath);
    std::error_code copyError;
    std::filesystem::copy(templateIncludePath,
                          renderedIncludePath,
                          std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
                          copyError);
    if (copyError) {
        logger.Error("Failed to copy include files from={} to={}, error={}",
                     templateIncludePath,
                     renderedIncludePath,
                     copyError.message());
        return false;
    }
    auto templateBinPath = templateDirectory / "Bin";
    auto renderedBinPath = projectParentPath / "Bin";
    logger.Info("Copying bin files from templatePath={} to renderedPath={}", templateBinPath, renderedBinPath);
    std::filesystem::copy(templateBinPath,
                          renderedBinPath,
                          std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
                          copyError);
    if (copyError) {
        logger.Error(
            "Failed to copy bin files from={} to={}, error={}", templateBinPath, renderedBinPath, copyError.message());
        return false;
    }
    return true;
}
