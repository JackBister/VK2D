#include "TypeChooser.h"

#include <imgui.h>

#include "Serialization/Deserializable.h"

TypeChooser::TypeFilter TypeChooser::COMPONENT_TYPE_FILTER = [](SerializedObjectSchema schema) {
    return schema.GetFlags().count(SerializedObjectFlag::IS_COMPONENT) > 0;
};

TypeChooser::TypeChooser(std::string title, TypeFilter typeFilter) : title(title), typeFilter(typeFilter)
{
    UpdateAvailableTypes();
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
    UpdateAvailableTypes();
    ImGui::OpenPopup(title.c_str());
}

void TypeChooser::UpdateAvailableTypes()
{
    auto const & deserialzables = Deserializable::Map();

    typeNames.clear();
    for (auto const & kv : Deserializable::Map()) {
        bool includeSchema = true;
        if (typeFilter) {
            includeSchema = typeFilter(kv.second->GetSchema());
        }
        if (includeSchema) {
            typeNames.push_back((char *)kv.first.c_str());
        }
    }
}