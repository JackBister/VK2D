#include "ProjectCreator.h"

#include <array>

#include "ComponentCreator.h"
#include "Logging/Logger.h"
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
        logger->Errorf("STUB: PartialProjectDeserializer::Deserialize");
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
        logger->Errorf("Failed to create project: given object does not match the PartialProject schema. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger->Errorf("\t%s: %s", err.first.c_str(), err.second.c_str());
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
        logger->Errorf("Failed to create project: failed to deserialize project after adding DLL and startingScene");
        return false;
    }
    auto project = projectOpt.value();

    bool writeProjectSuccess = WriteProjectFile(projectPath, project);
    if (!writeProjectSuccess) {
        return false;
    }

    bool writeComponentSuccess = WriteComponentFiles(projectParentPath);
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
        logger->Errorf("Failed to create build directory at path=%ls", renderedBuildPath.c_str());
        return false;
    }

    return true;
}

bool ProjectCreator::WriteProjectFile(std::filesystem::path projectPath, Project p)
{
    logger->Infof(
        "Writing project file to path=%ls for project with name=%s", projectPath.c_str(), p.GetName().c_str());
    auto serializedProject = p.Serialize();
    auto serializedProjectString = serializer->Serialize(serializedProject);

    bool projectWriteSuccess = WriteToFile(projectPath, serializedProjectString);
    if (!projectWriteSuccess) {
        logger->Errorf("Failed to write project with name=%s to path=%ls", p.GetName().c_str(), projectPath.c_str());
        return false;
    }
    return true;
}

bool ProjectCreator::WriteComponentFiles(std::filesystem::path projectParentPath)
{
    auto scriptPath = projectParentPath / "Scripts";
    logger->Infof("Writing component files to path=%ls", scriptPath.c_str());
    std::filesystem::create_directories(scriptPath);
    bool componentSuccess = componentCreator->CreateComponentCode(
        scriptPath, "MyComponent", {{.type = SerializedValueType::DOUBLE, .name = "myProperty"}});
    if (!componentSuccess) {
        logger->Errorf("Failed to generate template component at path=%ls", projectParentPath.c_str());
        return false;
    }
    return true;
}

bool ProjectCreator::WriteTemplateFile(std::string templateFileName, std::filesystem::path projectParentPath,
                                       SerializedObject & templateCtx)
{
    auto templatePath = templateDirectory / templateFileName;
    logger->Infof(
        "Rendering template from path=%ls to project at path=%ls", templatePath.c_str(), projectParentPath.c_str());
    if (templateFileName.ends_with(".tpl")) {
        std::optional<std::string> renderedOpt = templateRenderer->RenderTemplate(templatePath, templateCtx);
        if (!renderedOpt.has_value()) {
            logger->Errorf("Failed to render template with path=%ls", templatePath.c_str());
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
            logger->Errorf("Failed to write rendered template to path=%ls", renderedPath.c_str());
            return false;
        }
    } else {
        auto renderedPath = projectParentPath / templateFileName;
        std::filesystem::create_directories(renderedPath.parent_path());
        std::error_code copyError;
        bool copySuccess = std::filesystem::copy_file(
            templatePath, renderedPath, std::filesystem::copy_options::overwrite_existing, copyError);
        if (copyError) {
            logger->Errorf("Failed to copy template file from templatePath=%ls to renderedPath=%ls, error=%s",
                           templatePath.c_str(),
                           renderedPath.c_str(),
                           copyError.message().c_str());
            return false;
        }
        if (!copySuccess) {
            logger->Errorf("Failed to copy template file from templatePath=%ls to renderedPath=%ls",
                           templatePath.c_str(),
                           renderedPath.c_str());
        }
        return false;
    }
    return true;
}

bool ProjectCreator::CopyLibraryFiles(std::filesystem::path projectParentPath)
{
    auto templateIncludePath = templateDirectory / "Include";
    auto renderedIncludePath = projectParentPath / "Include";
    logger->Infof("Copying include files from templatePath=%ls to renderedPath=%ls",
                  templateIncludePath.c_str(),
                  renderedIncludePath.c_str());
    std::error_code copyError;
    std::filesystem::copy(templateIncludePath,
                          renderedIncludePath,
                          std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
                          copyError);
    if (copyError) {
        logger->Errorf("Failed to copy include files from=%ls to=%ls, error=%s",
                       templateIncludePath.c_str(),
                       renderedIncludePath.c_str(),
                       copyError.message().c_str());
        return false;
    }
    auto templateBinPath = templateDirectory / "Bin";
    auto renderedBinPath = projectParentPath / "Bin";
    logger->Infof("Copying bin files from templatePath=%ls to renderedPath=%ls",
                  templateBinPath.c_str(),
                  renderedBinPath.c_str());
    std::filesystem::copy(templateBinPath,
                          renderedBinPath,
                          std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
                          copyError);
    if (copyError) {
        logger->Errorf("Failed to copy bin files from=%ls to=%ls, error=%s",
                       templateBinPath.c_str(),
                       renderedBinPath.c_str(),
                       copyError.message().c_str());
        return false;
    }
    return true;
}
