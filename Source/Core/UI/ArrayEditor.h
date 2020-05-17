#pragma once

#include <optional>
#include <string>
#include <variant>

#include "Core/Serialization/SerializedObjectSchema.h"
#include "TypeChooser.h"

class EditorInstance;

class ArrayEditor
{
public:
    enum class ArrayEditorType { OBJECT = 0, VALUE = 1 };

    ArrayEditor(std::string name, ArrayEditorType type, std::optional<SerializedObjectSchema> schema,
                std::optional<SerializedValueType> valueType);

    void Draw();
    SerializedArray Build();

private:
    int size = 0;
    std::string name;
    ArrayEditorType type;
    std::variant<std::vector<std::unique_ptr<EditorInstance>>, std::vector<SerializedValue>> contents;
    std::optional<SerializedObjectSchema> schema;
    std::optional<SerializedValueType> valueType;

    std::optional<SerializedObjectSchema> newMemberSchema;
    TypeChooser typeChooser;
    size_t typeChooserActiveForIndex;

    void Resize();
};