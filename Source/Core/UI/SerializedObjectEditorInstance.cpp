#include "SerializedObjectEditorInstance.h"

#include <imgui.h>

#include "ArrayEditor.h"
#include "Core/Deserializable.h"
#include "Core/Util/Imgui_InputText.h"

EditorInstance::EditorInstance(SerializedObjectSchema schema) : schema(schema)
{
    auto const properties = schema.GetProperties();
    for (auto const & prop : properties) {
        auto objectSchemaOpt = Deserializable::GetSchema(prop.GetObjectSchemaName());
        if (prop.GetType() == SerializedValueType::BOOL) {
            values[prop.GetName()] = false;
        } else if (prop.GetType() == SerializedValueType::DOUBLE) {
            values[prop.GetName()] = 0.0;
        } else if (prop.GetType() == SerializedValueType::STRING) {
            values[prop.GetName()] = std::string("");
        } else if (prop.GetType() == SerializedValueType::OBJECT && objectSchemaOpt.has_value()) {
            objects[prop.GetName()] = std::make_unique<EditorInstance>(objectSchemaOpt.value());
        } else if (prop.GetType() == SerializedValueType::ARRAY && prop.GetArrayType().has_value()) {
            arrays[prop.GetName()] = std::make_unique<ArrayEditor>(prop.GetName(),
                                                                   prop.GetArrayType() == SerializedValueType::OBJECT
                                                                       ? ArrayEditor::ArrayEditorType::OBJECT
                                                                       : ArrayEditor::ArrayEditorType::VALUE,
                                                                   objectSchemaOpt,
                                                                   prop.GetArrayType());
        }
    }
}

void EditorInstance::Draw()
{
    auto const properties = schema.GetProperties();

    for (auto const & prop : properties) {
        DrawProperty(prop);
    }
}

void EditorInstance::DrawProperty(SerializedPropertySchema const & prop)
{
    auto objectSchemaOpt = Deserializable::GetSchema(prop.GetObjectSchemaName());
    if (prop.GetType() == SerializedValueType::BOOL) {
        ImGui::Text(prop.GetName().c_str());
        ImGui::SameLine();
        ImGui::PushID((int)&values[prop.GetName()]);
        ImGui::Checkbox("##hidelabel", &std::get<bool>(values[prop.GetName()]));
        ImGui::PopID();
    } else if (prop.GetType() == SerializedValueType::DOUBLE) {
        ImGui::Text(prop.GetName().c_str());
        ImGui::SameLine();
        ImGui::PushID((int)&values[prop.GetName()]);
        ImGui::InputDouble("##hidelabel", &std::get<double>(values[prop.GetName()]));
        ImGui::PopID();
    } else if (prop.GetType() == SerializedValueType::STRING) {
        ImGui::Text(prop.GetName().c_str());
        ImGui::SameLine();
        ImGui::PushID((int)&values[prop.GetName()]);
        ImGui_InputText("##hidelabel", &std::get<std::string>(values[prop.GetName()]));
        ImGui::PopID();
    } else if (prop.GetType() == SerializedValueType::OBJECT && objectSchemaOpt.has_value()) {
        if (ImGui::TreeNode((prop.GetName() + " <" + objectSchemaOpt.value().GetName() + '>').c_str())) {
            objects[prop.GetName()]->Draw();
            ImGui::TreePop();
        }
    } else if (prop.GetType() == SerializedValueType::ARRAY && prop.GetArrayType().has_value()) {
        arrays[prop.GetName()]->Draw();
    }
}

SerializedObject EditorInstance::Build()
{
    auto builder = SerializedObject::Builder();

    for (auto const & kv : arrays) {
        builder.WithArray(kv.first, kv.second->Build());
    }

    for (auto const & kv : objects) {
        builder.WithObject(kv.first, kv.second->Build());
    }

    for (auto const & kv : values) {
        builder.WithValue(kv.first, kv.second);
    }

    builder.WithString("type", schema.GetName());

    return builder.Build();
}
