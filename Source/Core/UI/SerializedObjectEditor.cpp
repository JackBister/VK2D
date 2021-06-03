#include "SerializedObjectEditor.h"

#include <memory>
#include <unordered_map>

#include <imgui.h>

#include "Logging/Logger.h"
#include "Serialization/JsonSerializer.h"
#include "SerializedObjectEditorInstance.h"
#include "Util/Imgui_InputText.h"

static auto const logger = Logger::Create("SerializedObjectEditor");

bool SerializedObjectEditor::Draw(SerializedObject * obj)
{
    if (!editorInstance.has_value()) {
        return false;
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
            *obj = editorInstance.value().Build();
            ImGui::EndPopup();
            return true;
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
    return false;
}

void SerializedObjectEditor::Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory)
{
    ImGui::OpenPopup(title.c_str());
    editorInstance = EditorInstance(schema, workingDirectory);
    hasSetSize = false;
}
