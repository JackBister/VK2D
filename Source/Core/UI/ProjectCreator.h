#pragma once

#include <filesystem>

#include "Core/Resources/Project.h"

class ComponentCreator;
class FileSlurper;
class Serializer;
class TemplateRenderer;

class ProjectCreator
{
public:
    ProjectCreator(ComponentCreator * componentCreator, FileSlurper * fileSlurper, Serializer * serializer,
                   TemplateRenderer * templateRenderer,
                   std::filesystem::path templateDirectory = "Templates/NewProject")
        : componentCreator(componentCreator), fileSlurper(fileSlurper), serializer(serializer),
          templateRenderer(templateRenderer), templateDirectory(templateDirectory)
    {
    }

    bool CreateProject(std::filesystem::path projectPath, SerializedObject partialProject);

    bool CopyLibraryFiles(std::filesystem::path projectParentPath);

private:
    bool WriteProjectFile(std::filesystem::path projectPath, Project p);
    bool WriteComponentFiles(std::filesystem::path projectParentPath);
    bool WriteTemplateFile(std::string templateFileName, std::filesystem::path projectParentPath,
                           SerializedObject & templateCtx);

    ComponentCreator * componentCreator;
    FileSlurper * fileSlurper;
    Serializer * serializer;
    TemplateRenderer * templateRenderer;

    std::filesystem::path templateDirectory;
};