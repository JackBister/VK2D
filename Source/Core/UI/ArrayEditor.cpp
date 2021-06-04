#include "ArrayEditor.h"

#include <imgui.h>

#include "Core/UI/SerializedObjectEditorInstance.h"
#include "Logging/Logger.h"
#include "Util/Imgui_InputText.h"

static auto const logger = Logger::Create("ArrayEditor");

ArrayEditor::ArrayEditor(std::string name, ArrayEditorType type, std::optional<SerializedObjectSchema> schema,
                         std::optional<SerializedValueType> valueType, std::filesystem::path workingDirectory,
                         SerializedPropertyFlags flags)
    : name(name), type(type), schema(schema), valueType(valueType),
      typeChooser(name + ".type", flags.HasFlag(SerializedPropertyFlagType::IS_COMPONENT)
                                      ? TypeChooser::COMPONENT_TYPE_FILTER
                                      : nullptr),
      workingDirectory(workingDirectory), flags(flags)
{
    if (type == ArrayEditorType::OBJECT) {
        contents = std::vector<std::unique_ptr<EditorInstance>>();
    } else {
        contents = std::vector<SerializedValue>();
    }

    fileBrowser.SetPwd(workingDirectory);
}

ArrayEditor::ArrayEditor(std::string name, ArrayEditorType type, std::optional<SerializedObjectSchema> schema,
                         std::optional<SerializedValueType> valueType, std::filesystem::path workingDirectory,
                         SerializedArray arr, SerializedPropertyFlags flags)
    : name(name), type(type), schema(schema), valueType(valueType),
      typeChooser(name + ".type", flags.HasFlag(SerializedPropertyFlagType::IS_COMPONENT)
                                      ? TypeChooser::COMPONENT_TYPE_FILTER
                                      : nullptr),
      workingDirectory(workingDirectory), flags(flags)
{
    if (type == ArrayEditorType::OBJECT) {
        contents = std::vector<std::unique_ptr<EditorInstance>>();
        if (schema.has_value()) {
            for (auto const & obj : arr) {
                // Absolutely disgusting.
                std::get<std::vector<std::unique_ptr<EditorInstance>>>(contents).emplace_back(
                    std::make_unique<EditorInstance>(
                        schema.value(), workingDirectory, std::get<SerializedObject>(obj)));
            }
        } else {
            logger->Warnf("Creating ArrayEditor with name=%s, type was OBJECT but no schema was provided. The original "
                          "values in the array will be lost.",
                          name.c_str());
        }
    } else {
        contents = arr;
    }

    fileBrowser.SetPwd(workingDirectory);
}

void ArrayEditor::Draw()
{
    if (ImGui::TreeNode(name.c_str())) {
        ImGui::Text("length");
        ImGui::SameLine();
        ImGui::PushID((int)&size);
        ImGui::InputInt("##hidelabel", &size);
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("OK")) {
            Resize();
        }

        if (type == ArrayEditorType::OBJECT) {
            auto & objects = std::get<0>(contents);
            for (size_t i = 0; i < objects.size(); ++i) {
                std::string label = '[' + std::to_string(i) + ']';
                if (objects[i]) {
                    label += " <" + objects[i]->GetTypeName() + '>';
                    if (ImGui::TreeNode(label.c_str())) {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Change type")) {
                            typeChooser.Open();
                            typeChooserActiveForIndex = i;
                        }
                        objects[i]->Draw();
                        ImGui::TreePop();
                    } else {
                        if (!schema.has_value()) {
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Change type")) {
                                typeChooser.Open();
                                typeChooserActiveForIndex = i;
                            }
                        }
                    }
                } else {
                    ImGui::Text(label.c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("Create")) {
                        typeChooser.Open();
                        typeChooserActiveForIndex = i;
                    }
                }
            }
        } else {
            auto & values = std::get<1>(contents);
            for (size_t i = 0; i < values.size(); ++i) {
                ImGui::Text(('[' + std::to_string(i) + ']').c_str());
                ImGui::SameLine();
                ImGui::PushID((int)&values[i]);
                if (valueType.value() == SerializedValueType::BOOL) {
                    ImGui::Checkbox("##hidelabel", &std::get<bool>(values[i]));
                } else if (valueType.value() == SerializedValueType::DOUBLE) {
                    ImGui::InputDouble("##hidelabel", &std::get<double>(values[i]));
                } else if (valueType.value() == SerializedValueType::STRING) {
                    if (flags.HasFlag(SerializedPropertyFlagType::IS_FILE_PATH)) {
                        ImGui::Text("%s", std::get<std::string>(values[i]).c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Pick")) {
                            fileBrowser.Open();
                            fileBrowserActiveForIndex = i;
                        }
                    } else {
                        ImGui_InputText("##hidelabel", &std::get<std::string>(values[i]));
                    }
                }
                ImGui::PopID();
            }
        }

        if (typeChooser.Draw(&newMemberSchema)) {
            auto & objects = std::get<0>(contents);
            objects[typeChooserActiveForIndex] =
                std::make_unique<EditorInstance>(newMemberSchema.value(), workingDirectory);
        }
        ImGui::TreePop();
    }

    DrawFileBrowser();
}

void ArrayEditor::DrawFileBrowser()
{

    fileBrowser.Display();
    if (fileBrowser.HasSelected()) {
        if (!fileBrowserActiveForIndex.has_value()) {
            logger->Warnf("Unexpected state when editing array with name=%s, fileBrowser was used but "
                          "fileBrowserActiveForIndex was not set",
                          name.c_str());
        } else if (valueType != SerializedValueType::STRING) {
            logger->Warnf("Unexpected state when editing array with name=%s, fileBrowser was used but valueType "
                          "was not STRING. valueType was=%zu",
                          name.c_str(),
                          valueType);
        } else {
            auto relativePath = std::filesystem::relative(fileBrowser.GetSelected(), workingDirectory);
            std::get<1>(contents)[fileBrowserActiveForIndex.value()] = relativePath.string();
        }
        fileBrowserActiveForIndex = std::nullopt;
        fileBrowser.Close();
    }
}

SerializedArray ArrayEditor::Build()
{
    SerializedArray ret;
    if (type == ArrayEditorType::OBJECT) {
        auto & objects = std::get<0>(contents);
        for (size_t i = 0; i < objects.size(); ++i) {
            if (!objects[i]) {
                logger->Warnf("Object at index=%zu in array was not initialized, this index will be skipped.", i);
            } else {
                ret.push_back(objects[i]->Build());
            }
        }
    } else {
        auto & values = std::get<1>(contents);
        for (auto const & val : values) {
            ret.push_back(val);
        }
    }
    return ret;
}

void ArrayEditor::Resize()
{
    size_t oldSize;
    if (type == ArrayEditorType::OBJECT) {
        auto & objects = std::get<0>(contents);
        oldSize = objects.size();
        objects.resize(size);
        if (size > oldSize) {
            if (schema.has_value()) {
                for (size_t i = oldSize; i < size; ++i) {
                    objects[i] = std::make_unique<EditorInstance>(schema.value(), workingDirectory);
                }
            } else {
                for (size_t i = oldSize; i < size; ++i) {
                    objects[i] = nullptr;
                }
            }
        }
    } else {
        if (!valueType.has_value()) {
            logger->Errorf("Cannot resize array with name=%s because valueType was not present. The schema may have "
                           "been incorrectly created.",
                           name.c_str());
            return;
        }
        auto & values = std::get<1>(contents);
        oldSize = values.size();
        values.resize(size);
        if (size > oldSize) {
            switch (valueType.value()) {
            case SerializedValueType::BOOL:
                for (size_t i = oldSize; i < size; ++i) {
                    values[i] = false;
                }
                break;
            case SerializedValueType::DOUBLE:
                for (size_t i = oldSize; i < size; ++i) {
                    values[i] = 0.0;
                }
                break;
            case SerializedValueType::STRING:
                for (size_t i = oldSize; i < size; ++i) {
                    values[i] = std::string("");
                }
                break;
            default:
                logger->Errorf("ArrayEditor does not support this type of array. valueType=%zu", valueType.value());
                values.resize(oldSize);
                break;
            }
        }
    }
}