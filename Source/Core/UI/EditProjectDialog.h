#pragma once

#include <filesystem>
#include <optional>

#include <imgui.h>

#include <imfilebrowser.h>

#include "Core/Resources/Project.h"
#include "SerializedObjectEditorInstance.h"

struct EditProjectDialogResult {
    bool isNewProject;
    std::filesystem::path path;
    SerializedObject project;
};

class EditProjectDialog
{
public:
    std::optional<EditProjectDialogResult> Draw();
    void Open(std::filesystem::path location, Project project);
    void OpenNew(std::filesystem::path startingPath);

private:
    ImGui::FileBrowser locationBrowser{ImGuiFileBrowserFlags_EnterNewFilename};
    std::optional<EditorInstance> editor;

    std::optional<std::string> errorMessage;

    std::optional<std::filesystem::path> location;

    bool isPartialProject = false;
};