#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/DllExport.h"
#include "SerializedValueType.h"

class SerializedValue;

typedef std::vector<std::pair<std::string, SerializedValue>> ValueMap;

class EAPI SerializedObject
{
public:
    class EAPI Builder
    {
    public:
        Builder & WithBool(std::string const & key, bool value);
        Builder & WithNumber(std::string const & key, double value);
        Builder & WithString(std::string const & key, std::string const & value);
        Builder & WithObject(std::string const & key, SerializedObject value);
        Builder & WithArray(std::string const & key, std::vector<SerializedValue> const & value);
        Builder & WithValue(std::string const & key, SerializedValue value);
        SerializedObject Build();

    private:
        ValueMap values;
    };

    std::optional<bool> GetBool(std::string const &) const;
    std::optional<double> GetNumber(std::string const &) const;
    std::optional<std::string> GetString(std::string const &) const;
    std::optional<SerializedObject> GetObject(std::string const &) const;
    std::optional<std::vector<SerializedValue>> GetArray(std::string const &) const;

    std::optional<SerializedValueType> GetType(std::string const &) const;

    ValueMap GetValues() const;

private:
    SerializedObject(ValueMap const & values);

    ValueMap::const_iterator FindByKey(std::string key) const;

    ValueMap values;
};
