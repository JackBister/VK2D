#include "SerializedObjectEditor.h"

#include <imgui.h>

#include "Logging/Logger.h"

static auto const logger = Logger::Create("SerializedObjectEditor");

std::optional<SerializedObject> SerializedObjectEditor::Draw()
{
    if (!editorInstance.has_value()) {
        return std::nullopt;
    }
    if (ImGui::BeginPopupModal(title.c_str())) {
        if (!hasSetSize) {
            ImGui::SetWindowSize(title.c_str(), ImVec2(500, 400));
            hasSetSize = true;
        }

        editorInstance.value().Draw();

        if (errorMessage.has_value()) {
            ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "%s", errorMessage.value().c_str());
        }

        if (ImGui::Button("OK")) {
            ImGui::EndPopup();
            return editorInstance.value().Build();
        }
        ImGui::SameLine();

        if (closeOnNextFrame) {
            closeOnNextFrame = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Close")) {
            closeOnNextFrame = true;
        }
        ImGui::EndPopup();
    }
    return std::nullopt;
}

void SerializedObjectEditor::Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory)
{
    ImGui::OpenPopup(title.c_str());
    editorInstance = EditorInstance(schema, workingDirectory);
    hasSetSize = false;
}

void SerializedObjectEditor::Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory,
                                  SerializedObject startingObject)
{
    ImGui::OpenPopup(title.c_str());
    editorInstance = EditorInstance(schema, workingDirectory, startingObject);
    hasSetSize = false;
}
