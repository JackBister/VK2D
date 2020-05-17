#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "Core/Serialization/SerializedObjectSchema.h"

class TypeChooser
{
public:
    TypeChooser(std::string title);

    bool Draw(std::optional<SerializedObjectSchema> * result);
    void Open();

private:
    std::string title;

    int selection = 0;
    std::vector<char *> typeNames;
};