#include "NewComponentTypeEditor.h"

#include "Logging/Logger.h"
#include "Serialization/Deserializable.h"
#include "Serialization/SerializedObjectSchema.h"

static auto const logger = Logger::Create("NewComponentTypeEditor");

static SerializedObjectSchema COMPONENT_PROPERTY_SCHEMA(
    "ComponentProperty",
    {SerializedPropertySchema::Required("type", SerializedValueType::STRING,
                                        SerializedPropertyFlags({StringEnumFlag({"BOOL", "DOUBLE", "STRING"})})),
     SerializedPropertySchema::Required("name", SerializedValueType::STRING)});

class ComponentPropertyDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() override { return COMPONENT_PROPERTY_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) override
    {
        logger->Errorf("STUB: ComponentPropertyDeserializer::Deserialize");
        return nullptr;
    }
};

DESERIALIZABLE_IMPL(ComponentProperty, new ComponentPropertyDeserializer())

static SerializedObjectSchema
    NEW_COMPONENT_SCHEMA("NewComponentType",
                         {SerializedPropertySchema::Required("directory", SerializedValueType::STRING,
                                                             SerializedPropertyFlags({IsFilePathFlag()})),
                          SerializedPropertySchema::Required("name", SerializedValueType::STRING),
                          SerializedPropertySchema::RequiredObjectArray("properties", "ComponentProperty")});

class NewComponentTypeDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() override { return NEW_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) override
    {
        logger->Errorf("STUB: NewComponentTypeDeserializer::Deserialize");
        return nullptr;
    }
};

DESERIALIZABLE_IMPL(NewComponentTypeProperty, new NewComponentTypeDeserializer())

std::optional<SerializedObject> NewComponentTypeEditor::Draw()
{
    auto res = editor.Draw();
    if (res.has_value()) {
        editor.ClearErrorMessage();
        auto & obj = res.value();
        if (!obj.GetString("name").has_value() || obj.GetString("name").value().empty()) {
            editor.SetErrorMessage("name cannot be empty");
            return std::nullopt;
        }
    }
    return res;
}

void NewComponentTypeEditor::Open(std::filesystem::path projectPath)
{
    editor.Open(NEW_COMPONENT_SCHEMA,
                projectPath,
                SerializedObject::Builder().WithString("directory", (projectPath / "Scripts").string()).Build());
}

void NewComponentTypeEditor::Close()
{
    editor.Close();
}
