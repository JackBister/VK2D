#include "SerializedObjectEditor.h"

#include <memory>
#include <unordered_map>

#include <imgui.h>

#include "Core/Logging/Logger.h"
#include "Core/Serialization/JsonSerializer.h"
#include "Core/Util/Imgui_InputText.h"
#include "SerializedObjectEditorInstance.h"

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

        if (ImGui::Button("ok")) {
            *obj = editorInstance.value().Build();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return false;
}

void SerializedObjectEditor::Open(SerializedObjectSchema schema)
{
    ImGui::OpenPopup(title.c_str());
    editorInstance = EditorInstance(schema);
    hasSetSize = false;
}
