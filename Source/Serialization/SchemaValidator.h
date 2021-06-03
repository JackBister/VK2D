#pragma once

#include <string>
#include <unordered_map>

#include "SerializedObject.h"
#include "SerializedObjectSchema.h"

namespace SchemaValidator
{
struct Result {
    bool isValid;
    std::unordered_map<std::string, std::string> propertyErrors;

    void AddError(std::string propertyName, std::string errorMessage)
    {
        isValid = false;
        propertyErrors[propertyName] = errorMessage;
    }
};

Result Validate(SerializedObjectSchema const & schema, SerializedObject const & object);
}
