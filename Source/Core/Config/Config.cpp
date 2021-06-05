#include "Config.h"

#include <cstdlib>
#include <unordered_map>

#include "Console/Console.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("Config");

namespace Config
{
std::unordered_map<std::string, bool> boolValues;
std::unordered_map<std::string, float> floatValues;
std::unordered_map<std::string, int> intValues;
std::unordered_map<std::string, std::string> stringValues;

void Init()
{
    CommandDefinition cfgGetDefinition(
        "cfg_get", "cfg_get <property> - gets the value of the property named <property>", 1, [](auto args) {
            auto property = args[0];
            if (Config::boolValues.find(property) != Config::boolValues.end()) {
                if (boolValues.at(property)) {
                    logger.Info("true");
                } else {
                    logger.Info("false");
                }
            } else if (Config::floatValues.find(property) != Config::floatValues.end()) {
                logger.Info("{}", floatValues.at(property));
            } else if (Config::intValues.find(property) != Config::intValues.end()) {
                logger.Info("{}", intValues.at(property));
            } else if (Config::stringValues.find(property) != Config::stringValues.end()) {
                logger.Info("{}", stringValues.at(property));
            } else {
                logger.Error("Property '{}' not found", property);
            }
        });
    Console::RegisterCommand(cfgGetDefinition);
    CommandDefinition cfgSetDefinition(
        "cfg_set", "cfg_set <property> <value> - sets the property named <property> to <value>", 2, [](auto args) {
            auto property = args[0];
            auto value = args[1];
            if (boolValues.find(property) != boolValues.end()) {
                if (value != "false" && value != "true") {
                    logger.Error("Property '{}' is a bool, but given value '{}' is not a bool", property, value);
                    return;
                }
                boolValues.at(property) = value == "true";
            } else if (floatValues.find(property) != floatValues.end()) {
                auto convertedValue = std::strtof(value.c_str(), nullptr);
                floatValues.at(property) = convertedValue;
            } else if (intValues.find(property) != intValues.end()) {
                auto convertedValue = std::strtol(value.c_str(), nullptr, 0);
                if (convertedValue == 0 && value != "0") {
                    logger.Error("Invalid int value {} for property '{}'", value, property);
                }
                intValues.at(property) = convertedValue;
            } else if (stringValues.find(property) != stringValues.end()) {
                stringValues.at(property) = value;
            } else {
                logger.Error("Property '{}' not found", property);
            }
        });
    Console::RegisterCommand(cfgSetDefinition);
}

DynamicBoolProperty AddBool(std::string const & name, bool value)
{
    boolValues.insert_or_assign(name, value);
    return DynamicBoolProperty(value, name);
}

DynamicFloatProperty AddFloat(std::string const & name, float value)
{
    floatValues.insert_or_assign(name, value);
    return DynamicFloatProperty(value, name);
}

DynamicIntProperty AddInt(std::string const & name, int value)
{
    intValues.insert_or_assign(name, value);
    return DynamicIntProperty(value, name);
}

DynamicStringProperty AddString(std::string const & name, std::string value)
{
    stringValues.insert_or_assign(name, value);
    return DynamicStringProperty(value, name);
}

DynamicBoolProperty GetBool(std::string const & name, bool defaultValue)
{
    return DynamicBoolProperty(defaultValue, name);
}

DynamicFloatProperty GetFloat(std::string const & name, float defaultValue)
{
    return DynamicFloatProperty(defaultValue, name);
}

DynamicIntProperty GetInt(std::string const & name, int defaultValue)
{
    return DynamicIntProperty(defaultValue, name);
}

DynamicStringProperty GetString(std::string const & name, std::string defaultValue)
{
    return DynamicStringProperty(defaultValue, name);
}
};

bool DynamicBoolProperty::Get() const
{
    if (Config::boolValues.find(propertyName) == Config::boolValues.end()) {
        return defaultValue;
    }
    return Config::boolValues.at(propertyName);
}

float DynamicFloatProperty::Get() const
{
    if (Config::floatValues.find(propertyName) == Config::floatValues.end()) {
        return defaultValue;
    }
    return Config::floatValues.at(propertyName);
}

int DynamicIntProperty::Get() const
{
    if (Config::intValues.find(propertyName) == Config::intValues.end()) {
        return defaultValue;
    }
    return Config::intValues.at(propertyName);
}

std::string DynamicStringProperty::Get() const
{
    if (Config::stringValues.find(propertyName) == Config::stringValues.end()) {
        return defaultValue;
    }
    return Config::stringValues.at(propertyName);
}
