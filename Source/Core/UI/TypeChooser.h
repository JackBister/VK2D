#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "Serialization/SerializedObjectSchema.h"

class TypeChooser
{
public:
    using TypeFilter = std::function<bool(SerializedObjectSchema)>;
    static TypeFilter COMPONENT_TYPE_FILTER;

    TypeChooser(std::string title, TypeFilter typeFilter = nullptr);

    bool Draw(std::optional<SerializedObjectSchema> * result);
    void Open();

private:
    void UpdateAvailableTypes();

    std::string title;
    TypeFilter typeFilter;

    int selection = 0;
    std::vector<char *> typeNames;
};