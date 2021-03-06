#include "EditProjectDialog_Private.h"

#include "Logging/Logger.h"
#include "Serialization/DeserializationContext.h"
#include "Serialization/SchemaValidator.h"

static auto const logger = Logger::Create("EditProjectDialog");

std::optional<EditProjectDialogResult> EditProjectDialog::Draw()
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

        if (location.has_value() && editor.has_value()) {
            editor.value().Draw();
        }

        locationBrowser.Display();
        if (locationBrowser.HasSelected()) {
            location = locationBrowser.GetSelected();
            SerializedObjectSchema schema = isPartialProject ? Deserializable::GetSchema("PartialProject").value()
                                                             : Deserializable::GetSchema("Project").value();
            if (editor.has_value()) {
                editor = EditorInstance(schema, locationBrowser.GetSelected().parent_path(), editor.value().Build());
            } else {
                logger.Warn(
                    "Unexpected state, editor was nullopt when setting location. Previous values will be lost.");
                editor = EditorInstance(schema, locationBrowser.GetSelected().parent_path());
            }
            locationBrowser.Close();
        }

        if (ImGui::Button("OK")) {
            if (!location.has_value()) {
                errorMessage = "Location must be specified";
            } else if (!editor.has_value()) {
                errorMessage = "Unknown error, !editor.has_value. Try closing this window and opening it again.";
            } else {
                SerializedObjectSchema schema = isPartialProject ? Deserializable::GetSchema("PartialProject").value()
                                                                 : Deserializable::GetSchema("Project").value();
                DeserializationContext ctx;
                ctx.workingDirectory = location.value().parent_path();
                SerializedObject obj = editor.value().Build();
                auto validationResult = SchemaValidator::Validate(schema, obj);
                if (!validationResult.isValid) {
                    errorMessage = "Schema validation failed, check console for details.";
                    logger.Error("Schema validation failed for PartialProject schema, errors:");
                    for (auto const & err : validationResult.propertyErrors) {
                        logger.Error("\t{}: {}", err.first, err.second);
                    }
                } else {
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return EditProjectDialogResult{
                        .isNewProject = isPartialProject, .path = location.value(), .project = obj};
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
    this->isPartialProject = false;
    this->location = location;
    locationBrowser.SetPwd(location);
    editor = EditorInstance(Deserializable::GetSchema("Project").value(), location.parent_path(), project.Serialize());
    ImGui::OpenPopup("Project");
}

void EditProjectDialog::OpenNew(std::filesystem::path startingPath)
{
    this->isPartialProject = true;
    locationBrowser.SetPwd(startingPath);
    editor = EditorInstance(Deserializable::GetSchema("PartialProject").value(), startingPath);
    ImGui::OpenPopup("Project");
}
