#pragma once

#include <string>

#include "SerializedValue.h"

class SerializedObjectSchema;

class EAPI SerializedPropertySchema
{
public:
    inline SerializedPropertySchema(std::string name, SerializedValueType type,
                                    std::optional<SerializedValueType> arrayType = {},
                                    SerializedObjectSchema * objectSchema = nullptr, bool isRequired = false,
                                    std::vector<std::string> requiredIfAbsent = {})
        : name(name), type(type), arrayType(arrayType), objectSchema(objectSchema), isRequired(isRequired)
    {
    }

    inline std::string GetName() const { return name; }
    inline SerializedValueType GetType() const { return type; }
    inline std::optional<SerializedValueType> GetArrayType() const { return arrayType; }
    inline SerializedObjectSchema * GetObjectSchema() const { return objectSchema; }
    inline bool IsRequired() const { return isRequired; }
    inline std::vector<std::string> GetRequiredIfAbsent() const { return requiredIfAbsent; }

private:
    std::string name;
    SerializedValueType type;
    // If type == ARRAY, this must be set and contain the type of the values contained in the array
    std::optional<SerializedValueType> arrayType;
    // If type == OBJECT, this should be set and contain the schema for the object
    // If type == ARRAY and arrayType == OBJECT, this should be set and contain the schema for the objects contained
    // in the array
    SerializedObjectSchema * objectSchema;
    bool isRequired;
    // The field is required if all field names in this vector are absent. This is useful for union types.
    // See CameraComponent for an example
    std::vector<std::string> requiredIfAbsent;
};

class EAPI SerializedObjectSchema
{
public:
    inline SerializedObjectSchema(std::vector<SerializedPropertySchema> properties) : properties(properties) {}

    inline std::vector<SerializedPropertySchema> GetProperties() const { return properties; }

private:
    std::vector<SerializedPropertySchema> properties;
};
