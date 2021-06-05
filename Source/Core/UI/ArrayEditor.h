#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <variant>

#include <imgui.h>

#include <imfilebrowser.h>

#include "Serialization/SerializedObjectSchema.h"
#include "TypeChooser.h"

class EditorInstance;

class ArrayEditor
{
public:
    enum class ArrayEditorType { OBJECT = 0, VALUE = 1 };

    ArrayEditor(std::string name, ArrayEditorType type, std::optional<SerializedObjectSchema> schema,
                std::optional<SerializedValueType> valueType, std::filesystem::path workingDirectory,
                SerializedPropertyFlags flags = SerializedPropertyFlags({}));
    ArrayEditor(std::string name, ArrayEditorType type, std::optional<SerializedObjectSchema> schema,
                std::optional<SerializedValueType> valueType, std::filesystem::path workingDirectory,
                SerializedArray arr, SerializedPropertyFlags flags = SerializedPropertyFlags({}));

    void Draw();
    SerializedArray Build();

private:
    int size = 0;
    std::string name;
    ArrayEditorType type;
    std::variant<std::vector<std::unique_ptr<EditorInstance>>, std::vector<SerializedValue>> contents;
    std::vector<int> enumSelections;
    std::optional<SerializedObjectSchema> schema;
    std::optional<SerializedValueType> valueType;
    SerializedPropertyFlags flags;

    std::optional<SerializedObjectSchema> newMemberSchema;
    TypeChooser typeChooser;
    size_t typeChooserActiveForIndex;

    ImGui::FileBrowser fileBrowser;
    std::optional<size_t> fileBrowserActiveForIndex;

    std::filesystem::path workingDirectory;

    void DrawFileBrowser();
    void Resize();
};