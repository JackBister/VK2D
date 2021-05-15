#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "Core/Serialization/SerializedObjectSchema.h"
#include "SerializedObjectEditorInstance.h"

class SerializedObjectEditor
{
public:
    SerializedObjectEditor(std::string title) : title(title) {}

    bool Draw(SerializedObject * obj);
    void Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory);

private:
    std::string title;
    std::optional<SerializedObjectSchema> schema;
    std::optional<EditorInstance> editorInstance;

    bool hasSetSize = false;
};
