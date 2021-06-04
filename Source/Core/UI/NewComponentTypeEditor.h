#pragma once

#include <optional>

#include "Serialization/SerializedObject.h"
#include "SerializedObjectEditor.h"

class NewComponentTypeEditor
{
public:
    std::optional<SerializedObject> Draw();
    void Open(std::filesystem::path projectPath);
    void Close();

private:
    SerializedObjectEditor editor{"New component"};
};
