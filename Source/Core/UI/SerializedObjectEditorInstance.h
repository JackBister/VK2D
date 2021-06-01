#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <imgui.h>

#include <imfilebrowser.h>

#include "ArrayEditor.h"
#include "Core/Serialization/SerializedObjectSchema.h"
#include "Core/Serialization/SerializedValue.h"

class EditorInstance
{
public:
    EditorInstance(SerializedObjectSchema schema, std::filesystem::path workingDirectory);
    EditorInstance(SerializedObjectSchema schema, std::filesystem::path workingDirectory, SerializedObject object);

    inline std::string GetTypeName() const { return schema.GetName(); }

    void Draw();

    SerializedObject Build();

private:
    SerializedObjectSchema schema;
    std::unordered_map<std::string, std::unique_ptr<ArrayEditor>> arrays;
    std::unordered_map<std::string, std::unique_ptr<EditorInstance>> objects;
    std::unordered_map<std::string, SerializedValue> values;

    std::filesystem::path workingDirectory;
    ImGui::FileBrowser fileBrowser;
    std::optional<std::string> currentFileProperty;

    void DrawProperty(SerializedPropertySchema const & prop);
};