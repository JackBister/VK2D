#include "EditProjectDialog.h"

#include "Core/Resources/Scene.h"
#include "Logging/Logger.h"

static auto const logger = Logger::Create("EditProjectDialog");

std::optional<std::pair<std::filesystem::path, Project>> EditProjectDialog::Draw()
{
    if (ImGui::BeginPopupModal("Project")) {
        ImGui::SetWindowSize("Project", ImVec2(800, 600), ImGuiCond_Once);
        if (errorMessage.has_value()) {
            ImGui::TextColored(ImVec4(255, 0, 0, 1), "%s", errorMessage.value().c_str());
        }
        ImGui::Text("File");
        ImGui::SameLine();
        if (location.has_value()) {
            ImGui::Text("%ls", location.value().c_str());
        }
        if (ImGui::Button("Pick")) {
            locationBrowser.Open();
        }

        // TODO: This UX is not great, but the alternative is having the user create the .scene file in a text editor
        // before being able to create the project...
        if (ImGui::Button("Create new scene file")) {
            if (location.has_value()) {
                startingSceneBrowser.SetPwd(location.value().parent_path());
            }
            startingSceneBrowser.Open();
        }

        if (location.has_value() && editor.has_value()) {
            editor.value().Draw();
        }

        startingSceneBrowser.Display();
        if (startingSceneBrowser.HasSelected()) {
            auto startingScenePath = startingSceneBrowser.GetSelected();
            Scene startingScene(startingScenePath.string(),
                                {},
                                {},
                                DeserializationContext{.workingDirectory = startingScenePath.parent_path()});
            startingScene.SerializeToFile(startingScenePath.string());
            startingSceneBrowser.Close();
        }

        locationBrowser.Display();
        if (locationBrowser.HasSelected()) {
            location = locationBrowser.GetSelected();
            if (editor.has_value()) {
                editor = EditorInstance(Deserializable::GetSchema("Project").value(),
                                        locationBrowser.GetSelected().parent_path(),
                                        editor.value().Build());
            } else {
                logger->Warnf(
                    "Unexpected state, editor was nullopt when setting location. Previous values will be lost.");
                editor = EditorInstance(Deserializable::GetSchema("Project").value(),
                                        locationBrowser.GetSelected().parent_path());
            }
            locationBrowser.Close();
        }

        if (ImGui::Button("OK")) {
            if (!location.has_value()) {
                errorMessage = "Location must be specified";
            } else if (!editor.has_value()) {
                errorMessage = "Unknown error, !editor.has_value. Try closing this window and opening it again.";
            } else {
                DeserializationContext ctx;
                ctx.workingDirectory = location.value().parent_path();
                SerializedObject obj = editor.value().Build();
                auto projectOpt = Project::Deserialize(&ctx, obj);
                if (!projectOpt.has_value()) {
                    errorMessage = "Failed to deserialize project, check console for details.";
                } else {
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return std::make_pair(location.value(), projectOpt.value());
                }
            }
        }
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return {};
}

void EditProjectDialog::Open(std::filesystem::path location, Project project)
{
    this->location = location;
    locationBrowser.SetPwd(location);
    editor = EditorInstance(Deserializable::GetSchema("Project").value(), location.parent_path(), project.Serialize());
    ImGui::OpenPopup("Project");
}

void EditProjectDialog::OpenNew(std::filesystem::path startingPath)
{
    locationBrowser.SetPwd(startingPath);
    editor = EditorInstance(Deserializable::GetSchema("Project").value(), startingPath);
    ImGui::OpenPopup("Project");
}
