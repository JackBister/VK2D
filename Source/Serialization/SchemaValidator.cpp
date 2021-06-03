#include "SchemaValidator.h"

#include "Deserializable.h"

// TODO: Additional details in error messages, like what index of the array is wrong
// TODO: Merge error messages from recursive calls
SchemaValidator::Result SchemaValidator::Validate(SerializedObjectSchema const & schema,
                                                  SerializedObject const & object)
{
    SchemaValidator::Result result{true};
    for (auto prop : schema.GetProperties()) {
        auto actualTypeOpt = object.GetType(prop.GetName());
        if (!actualTypeOpt.has_value()) {
            if (prop.IsRequired()) {
                result.AddError(prop.GetName(), "required but not present");
                continue;
            }
            auto requiredIfAbsent = prop.GetRequiredIfAbsent();
            if (requiredIfAbsent.size() > 0) {
                bool isAnyPresent = false;
                for (auto const & str : requiredIfAbsent) {
                    auto isPresent = object.GetType(str).has_value();
                    if (isPresent) {
                        isAnyPresent = true;
                        break;
                    }
                }
                if (!isAnyPresent) {
                    result.AddError(prop.GetName(), "conditionally required but not present");
                }
            }
            continue;
        }
        auto actualType = actualTypeOpt.value();
        if (actualType != prop.GetType()) {
            result.AddError(prop.GetName(), "wrong type");
        }

        auto objectSchemaOpt = Deserializable::GetSchema(prop.GetObjectSchemaName());
        if (actualType == SerializedValueType::ARRAY) {
            auto actualArray = object.GetArray(prop.GetName()).value();
            for (size_t i = 0; i < actualArray.size(); ++i) {
                auto const & val = actualArray[i];
                if (val.GetType() != prop.GetArrayType().value()) {
                    result.AddError(prop.GetName(), "wrong type");
                }
                if (val.GetType() == SerializedValueType::OBJECT && objectSchemaOpt.has_value()) {
                    auto res = Validate(objectSchemaOpt.value(), std::get<SerializedObject>(val));
                    if (!res.isValid) {
                        result.AddError(prop.GetName(), "object schema does not match");
                    }
                }
            }
        } else if (actualType == SerializedValueType::OBJECT && objectSchemaOpt.has_value()) {
            auto actualObject = object.GetObject(prop.GetName()).value();
            auto res = Validate(objectSchemaOpt.value(), actualObject);
            if (!res.isValid) {
                result.AddError(prop.GetName(), "object schema does not match");
            }
        }
    }
    return result;
}
