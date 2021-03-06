#include "SerializedObjectEditorInstance_Private.h"

#include <ThirdParty/imgui/imgui.h>

#undef GetObject

#include "ArrayEditor_Private.h"
#include "Imgui_InputText_Private.h"
#include "Logging/Logger.h"
#include "Serialization/Deserializable.h"

static auto const logger = Logger::Create("EditorInstance");

EditorInstance::EditorInstance(SerializedObjectSchema schema, std::filesystem::path workingDirectory)
    : schema(schema), workingDirectory(workingDirectory)
{
    auto const properties = schema.GetProperties();
    for (auto const & prop : properties) {
        auto objectSchemaOpt = Deserializable::GetSchema(prop.GetObjectSchemaName());
        if (prop.GetType() == SerializedValueType::BOOL) {
            values[prop.GetName()] = false;
        } else if (prop.GetType() == SerializedValueType::DOUBLE) {
            values[prop.GetName()] = 0.0;
        } else if (prop.GetType() == SerializedValueType::STRING) {
            auto key = prop.GetName();
            values[key] = std::string("");
            auto enumOpt = prop.GetFlags().GetFlag<StringEnumFlag>();
            if (enumOpt.has_value()) {
                auto options = enumOpt.value().GetValues();
                enumSelections[key] = 0;
                if (!options.empty()) {
                    values[key] = options[0];
                }
            }
        } else if (prop.GetType() == SerializedValueType::OBJECT && objectSchemaOpt.has_value()) {
            objects[prop.GetName()] = std::make_unique<EditorInstance>(objectSchemaOpt.value(), workingDirectory);
        } else if (prop.GetType() == SerializedValueType::ARRAY && prop.GetArrayType().has_value()) {
            arrays[prop.GetName()] = std::make_unique<ArrayEditor>(prop.GetName(),
                                                                   prop.GetArrayType() == SerializedValueType::OBJECT
                                                                       ? ArrayEditor::ArrayEditorType::OBJECT
                                                                       : ArrayEditor::ArrayEditorType::VALUE,
                                                                   objectSchemaOpt,
                                                                   prop.GetArrayType(),
                                                                   workingDirectory,
                                                                   prop.GetFlags());
        }
    }

    fileBrowser.SetPwd(workingDirectory);
}

EditorInstance::EditorInstance(SerializedObjectSchema schema, std::filesystem::path workingDirectory,
                               SerializedObject object)
    : EditorInstance(schema, workingDirectory)
{
    auto const properties = schema.GetProperties();
    for (auto const & prop : properties) {
        auto key = prop.GetName();
        auto objectSchemaOpt = Deserializable::GetSchema(prop.GetObjectSchemaName());
        auto type = prop.GetType();

        if (type == SerializedValueType::BOOL && object.GetBool(key).has_value()) {
            values[key] = object.GetBool(key).value();
        } else if (type == SerializedValueType::DOUBLE && object.GetNumber(key).has_value()) {
            values[key] = object.GetNumber(key).value();
        } else if (type == SerializedValueType::STRING && object.GetString(key).has_value()) {
            values[key] = object.GetString(key).value();
            if (prop.GetFlags().HasFlag(SerializedPropertyFlagType::IS_STRING_ENUM)) {
                enumSelections[key] = 0;
            }
        } else if (type == SerializedValueType::OBJECT && objectSchemaOpt.has_value() &&
                   object.GetObject(key).has_value()) {
            objects[key] = std::make_unique<EditorInstance>(
                objectSchemaOpt.value(), workingDirectory, object.GetObject(key).value());
        } else if (type == SerializedValueType::ARRAY && prop.GetArrayType().has_value() &&
                   object.GetArray(key).has_value()) {
            arrays[key] = std::make_unique<ArrayEditor>(prop.GetName(),
                                                        prop.GetArrayType() == SerializedValueType::OBJECT
                                                            ? ArrayEditor::ArrayEditorType::OBJECT
                                                            : ArrayEditor::ArrayEditorType::VALUE,
                                                        objectSchemaOpt,
                                                        prop.GetArrayType(),
                                                        workingDirectory,
                                                        object.GetArray(key).value(),
                                                        prop.GetFlags());
        }
    }
}

void EditorInstance::Draw()
{
    auto const properties = schema.GetProperties();

    for (auto const & prop : properties) {
        DrawProperty(prop);
    }

    fileBrowser.Display();
    if (fileBrowser.HasSelected()) {
        if (currentFileProperty.has_value()) {
            auto selected = fileBrowser.GetSelected();
            selected = std::filesystem::relative(selected, workingDirectory);
            values[currentFileProperty.value()] = selected.string();
        } else {
            logger.Warn("Unexpected state, file browser was open and a file was selected but no property was "
                        "currently being edited. Editing object with schema={}",
                        schema.GetName());
        }
        currentFileProperty = {};
        fileBrowser.Close();
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
        if (prop.GetFlags().HasFlag(SerializedPropertyFlagType::IS_FILE_PATH)) {
            ImGui::Text(std::get<std::string>(values[prop.GetName()]).c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Pick")) {
                fileBrowser.Open();
                currentFileProperty = prop.GetName();
            }
        } else if (prop.GetFlags().HasFlag(SerializedPropertyFlagType::IS_STRING_ENUM)) {
            auto key = prop.GetName();
            auto enumOpt = prop.GetFlags().GetFlag<StringEnumFlag>();
            if (enumOpt.has_value()) {
                auto selected = enumSelections[key];
                auto options = enumOpt.value().GetValues();
                if (!options.empty()) {
                    if (selected > options.size()) {
                        enumSelections[key] = selected = 0;
                        values[key] = options[0];
                    }
                    auto selectedString = options[selected];
                    if (ImGui::BeginCombo("##hidelabel", selectedString.c_str())) {
                        for (int i = 0; i < options.size(); ++i) {
                            bool isSelected = i == selected;
                            if (ImGui::Selectable(options[i].c_str(), isSelected)) {
                                enumSelections[key] = i;
                                values[key] = options[i];
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                } else {
                    logger.Warn("Property with key={}, there are no available enum options.", key);
                }
            } else {
                logger.Warn("Weird state, property with key={} HasFlag(IS_STRING_ENUM) but "
                            "GetFlag<StringEnumFlag> failed. Bug in SerializedPropertyFlags?",
                            key);
            }
        } else {
            ImGui_InputText("##hidelabel", &std::get<std::string>(values[prop.GetName()]));
        }
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
