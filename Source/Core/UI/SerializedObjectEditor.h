#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "Serialization/SerializedObjectSchema.h"
#include "SerializedObjectEditorInstance_Private.h"

class SerializedObjectEditor
{
public:
    SerializedObjectEditor(std::string title) : title(title) {}

    std::optional<SerializedObject> Draw();
    void Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory);
    void Open(SerializedObjectSchema schema, std::filesystem::path workingDirectory, SerializedObject startingObject);
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
