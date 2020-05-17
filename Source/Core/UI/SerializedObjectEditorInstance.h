#pragma once

#include <memory>
#include <unordered_map>

#include "ArrayEditor.h"
#include "Core/Serialization/SerializedObjectSchema.h"
#include "Core/Serialization/SerializedValue.h"

class EditorInstance
{
public:
    EditorInstance(SerializedObjectSchema schema);

    inline std::string GetTypeName() const { return schema.GetName(); }

    void Draw();

    SerializedObject Build();

private:
    SerializedObjectSchema schema;
    std::unordered_map<std::string, std::unique_ptr<ArrayEditor>> arrays;
    std::unordered_map<std::string, std::unique_ptr<EditorInstance>> objects;
    std::unordered_map<std::string, SerializedValue> values;

    void DrawProperty(SerializedPropertySchema const & prop);
};