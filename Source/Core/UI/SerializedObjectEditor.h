#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "Serialization/SerializedObjectSchema.h"
#include "SerializedObjectEditorInstance.h"

class SerializedObjectEditor
{
public:
    SerializedObjectEditor(std::string title) : title(title) {}

    bool Draw(SerializedObject * obj);
    void Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory);
    void Close() { closeOnNextFrame = true; }

    void SetErrorMessage(std::string newMessage) { this->errorMessage = newMessage; }
    void ClearErrorMessage() { errorMessage.reset(); }

private:
    std::string title;
    std::optional<SerializedObjectSchema> schema;
    std::optional<EditorInstance> editorInstance;

    bool hasSetSize = false;

    bool closeOnNextFrame = false;
    std::optional<std::string> errorMessage;
};
