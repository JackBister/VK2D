#include "SerializedObject.h"

#include <algorithm>

#include "SerializedValue.h"

SerializedObject::Builder & SerializedObject::Builder::WithBool(std::string const & key, bool value)
{
    WithValue(key, value);
    return *this;
}

SerializedObject::Builder & SerializedObject::Builder::WithNumber(std::string const & key, double value)
{
    WithValue(key, value);
    return *this;
}

SerializedObject::Builder & SerializedObject::Builder::WithString(std::string const & key, std::string const & value)
{
    WithValue(key, value);
    return *this;
}

SerializedObject::Builder & SerializedObject::Builder::WithObject(std::string const & key, SerializedObject value)
{
    WithValue(key, value);
    return *this;
}

SerializedObject::Builder & SerializedObject::Builder::WithArray(std::string const & key,
                                                                 std::vector<SerializedValue> const & value)
{
    WithValue(key, value);
    return *this;
}

SerializedObject::Builder & SerializedObject::Builder::WithValue(std::string const & key, SerializedValue value)
{
    values.push_back(std::make_pair(key, value));
    return *this;
}

SerializedObject SerializedObject::Builder::Build()
{
    return SerializedObject(values);
}

std::optional<bool> SerializedObject::GetBool(std::string const & key) const
{
    auto it = FindByKey(key);
    if (it == values.end() || it->second.index() != SerializedValue::BOOL) {
        return {};
    }
    return std::get<bool>(it->second);
}

std::optional<double> SerializedObject::GetNumber(std::string const & key) const
{
    auto it = FindByKey(key);
    if (it == values.end() || it->second.index() != SerializedValue::DOUBLE) {
        return {};
    }
    return std::get<double>(it->second);
}

std::optional<std::string> SerializedObject::GetString(std::string const & key) const
{
    auto it = FindByKey(key);
    if (it == values.end() || it->second.index() != SerializedValue::STRING) {
        return {};
    }
    return std::get<std::string>(it->second);
}

std::optional<SerializedObject> SerializedObject::GetObject(std::string const & key) const
{
    auto it = FindByKey(key);
    if (it == values.end() || it->second.index() != SerializedValue::OBJECT) {
        return {};
    }
    return std::get<SerializedObject>(it->second);
}

std::optional<std::vector<SerializedValue>> SerializedObject::GetArray(std::string const & key) const
{
    auto it = FindByKey(key);
    if (it == values.end() || it->second.index() != SerializedValue::ARRAY) {
        return {};
    }
    return std::get<SerializedArray>(it->second);
}

ValueMap SerializedObject::GetValues() const
{
    return values;
}

SerializedObject::SerializedObject(ValueMap const & values) : values(values) {}

ValueMap::const_iterator SerializedObject::FindByKey(std::string key) const
{
    return std::find_if(values.cbegin(), values.cend(), [key](auto p) { return p.first == key; });
}
