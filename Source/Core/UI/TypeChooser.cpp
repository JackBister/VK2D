#include "TypeChooser.h"

#include <imgui.h>

#include "Core/Deserializable.h"

TypeChooser::TypeChooser(std::string title) : title(title)
{
    auto const & deserialzables = Deserializable::Map();

    for (auto const & kv : Deserializable::Map()) {
        typeNames.push_back((char *)kv.first.c_str());
    }
}

bool TypeChooser::Draw(std::optional<SerializedObjectSchema> * result)
{
    if (ImGui::BeginPopupModal(title.c_str())) {
        ImGui::SetWindowSize(title.c_str(), ImVec2(300, 100));
        ImGui::Combo("##hidelabel", &selection, &typeNames[0], typeNames.size());

        if (ImGui::Button("OK")) {
            auto type = typeNames[selection];
            auto schema = Deserializable::Map()[type]->GetSchema();
            *result = schema;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }

        if (ImGui::Button("Close")) {
            *result = {};
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return false;
}

void TypeChooser::Open()
{
    ImGui::OpenPopup(title.c_str());
}
