#pragma once

#include <string>

class DynamicBoolProperty
{
public:
    DynamicBoolProperty(bool defaultValue, std::string propertyName)
        : defaultValue(defaultValue), propertyName(propertyName)
    {
    }

    bool Get() const;

private:
    bool defaultValue;
    std::string propertyName;
};

class DynamicFloatProperty
{
public:
    DynamicFloatProperty(float defaultValue, std::string propertyName)
        : defaultValue(defaultValue), propertyName(propertyName)
    {
    }

    float Get() const;

private:
    float defaultValue;
    std::string propertyName;
};

class DynamicIntProperty
{
public:
    DynamicIntProperty(int defaultValue, std::string propertyName)
        : defaultValue(defaultValue), propertyName(propertyName)
    {
    }

    int Get() const;

private:
    int defaultValue;
    std::string propertyName;
};

class DynamicStringProperty
{
public:
    DynamicStringProperty(std::string defaultValue, std::string propertyName)
        : defaultValue(defaultValue), propertyName(propertyName)
    {
    }

    std::string Get() const;

private:
    std::string defaultValue;
    std::string propertyName;
};

namespace Config
{
void Init();

DynamicBoolProperty AddBool(std::string const & name, bool initialValue);
DynamicFloatProperty AddFloat(std::string const & name, float initialValue);
DynamicIntProperty AddInt(std::string const & name, int initialValue);
DynamicStringProperty AddString(std::string const & name, std::string initialValue);
DynamicBoolProperty GetBool(std::string const & name, bool defaultValue);
DynamicFloatProperty GetFloat(std::string const & name, float defaultValue);
DynamicIntProperty GetInt(std::string const & name, int defaultValue);
DynamicStringProperty GetString(std::string const & name, std::string defaultValue);
};
