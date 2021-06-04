#pragma once

#include <filesystem>
#include <optional>
#include <tuple>

#include <imgui.h>

#include <imfilebrowser.h>

#include "Core/Resources/Project.h"
#include "SerializedObjectEditorInstance.h"

class EditProjectDialog
{
public:
    std::optional<std::pair<std::filesystem::path, SerializedObject>> Draw();
    void Open(std::filesystem::path location, Project project);
    void OpenNew(std::filesystem::path startingPath);

private:
    ImGui::FileBrowser locationBrowser{ImGuiFileBrowserFlags_EnterNewFilename};
    std::optional<EditorInstance> editor;

    std::optional<std::string> errorMessage;

    std::optional<std::filesystem::path> location;

    bool isPartialProject = false;
};