#pragma once

#include <string>
#include <unordered_set>

#include "SerializedValue.h"

class SerializedObjectSchema;

class IsComponentFlag
{
};

class IsFilePathFlag
{
};

class StringEnumFlag
{
public:
    StringEnumFlag(std::vector<std::string> const & values) : values(values) {}

    std::vector<std::string> const & GetValues() const { return values; }

private:
    std::vector<std::string> values;
};

enum class SerializedPropertyFlagType {
    IS_COMPONENT = 0,
    IS_FILE_PATH = 1,
    IS_STRING_ENUM = 2,
};

using SerializedPropertyFlag = std::variant<IsComponentFlag, IsFilePathFlag, StringEnumFlag>;

class SerializedPropertyFlags
{
public:
    SerializedPropertyFlags(std::vector<SerializedPropertyFlag> flags) : flags(flags) {}

    bool HasFlag(SerializedPropertyFlagType type) const
    {
        for (auto const & f : flags) {
            if (((SerializedPropertyFlagType)f.index()) == type) {
                return true;
            }
        }
        return false;
    }

    template <typename F>
    std::optional<F> GetFlag() const
    {
        for (auto const & f : flags) {
            if (std::holds_alternative<F>(f)) {
                return std::get<F>(f);
            }
        }

        return std::nullopt;
    }

private:
    std::vector<SerializedPropertyFlag> flags;
};

class EAPI SerializedPropertySchema
{
public:
    static SerializedPropertySchema Optional(std::string name, SerializedValueType type,
                                             SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, type, {}, "", false, {}, flags);
    }
    static SerializedPropertySchema OptionalArray(std::string name, SerializedValueType arrayType,
                                                  SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, SerializedValueType::ARRAY, arrayType, "", false, {}, flags);
    }
    static SerializedPropertySchema OptionalObject(std::string name, std::string objectSchemaName,
                                                   SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, SerializedValueType::OBJECT, {}, objectSchemaName, false, {}, flags);
    }
    static SerializedPropertySchema OptionalObjectArray(std::string name, std::string objectSchemaName,
                                                        SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(
            name, SerializedValueType::ARRAY, SerializedValueType::OBJECT, objectSchemaName, false, {}, flags);
    }

    static SerializedPropertySchema Required(std::string name, SerializedValueType type,
                                             SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, type, {}, "", true, {}, flags);
    }
    static SerializedPropertySchema RequiredArray(std::string name, SerializedValueType arrayType,
                                                  SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, SerializedValueType::ARRAY, arrayType, "", true, {}, flags);
    }
    static SerializedPropertySchema RequiredObject(std::string name, std::string objectSchemaName,
                                                   SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(name, SerializedValueType::OBJECT, {}, objectSchemaName, true, {}, flags);
    }
    static SerializedPropertySchema RequiredObjectArray(std::string name, std::string objectSchemaName,
                                                        SerializedPropertyFlags flags = SerializedPropertyFlags({}))
    {
        return SerializedPropertySchema(
            name, SerializedValueType::ARRAY, SerializedValueType::OBJECT, objectSchemaName, true, {}, flags);
    }

    inline SerializedPropertySchema(std::string name, SerializedValueType type,
                                    std::optional<SerializedValueType> arrayType = {},
                                    std::string objectSchemaName = "", bool isRequired = false,
                                    std::vector<std::string> requiredIfAbsent = {},
                                    SerializedPropertyFlags flags = SerializedPropertyFlags({}))
        : name(name), type(type), arrayType(arrayType), objectSchemaName(objectSchemaName), isRequired(isRequired),
          flags(flags)
    {
    }

    inline std::string GetName() const { return name; }
    inline SerializedValueType GetType() const { return type; }
    inline std::optional<SerializedValueType> GetArrayType() const { return arrayType; }
    inline std::string GetObjectSchemaName() const { return objectSchemaName; }
    inline bool IsRequired() const { return isRequired; }
    inline std::vector<std::string> GetRequiredIfAbsent() const { return requiredIfAbsent; }
    inline SerializedPropertyFlags GetFlags() const { return flags; }

private:
    std::string name;
    SerializedValueType type;
    // If type == ARRAY, this must be set and contain the type of the values contained in the array
    std::optional<SerializedValueType> arrayType;
    // If type == OBJECT, this should be set and contain the name of the schema for the object
    // If type == ARRAY and arrayType == OBJECT, this should be set and contain the name of the schema for the objects
    // contained in the array
    std::string objectSchemaName;
    bool isRequired;
    // The field is required if all field names in this vector are absent. This is useful for union types.
    // See CameraComponent for an example
    std::vector<std::string> requiredIfAbsent;
    SerializedPropertyFlags flags;
};

enum class SerializedObjectFlag {
    IS_COMPONENT,
};

class EAPI SerializedObjectSchema
{
public:
    inline SerializedObjectSchema(std::string name, std::vector<SerializedPropertySchema> properties,
                                  std::unordered_set<SerializedObjectFlag> flags = {})
        : name(name), properties(properties), flags(flags)
    {
    }

    inline std::string GetName() const { return name; }
    inline std::vector<SerializedPropertySchema> GetProperties() const { return properties; }
    inline std::unordered_set<SerializedObjectFlag> GetFlags() const { return flags; }

private:
    std::string name;
    std::vector<SerializedPropertySchema> properties;
    std::unordered_set<SerializedObjectFlag> flags;
};
