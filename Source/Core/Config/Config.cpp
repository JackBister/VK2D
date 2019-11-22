#include "Config.h"

#include <cstdlib>
#include <unordered_map>

#include "Core/Console/Console.h"
#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("Config");

namespace Config
{
std::unordered_map<std::string, bool> boolValues;
std::unordered_map<std::string, int> intValues;
std::unordered_map<std::string, std::string> stringValues;

void Init()
{
    CommandDefinition cfgGetDefinition(
        "cfg_get", "cfg_get <property> - gets the value of the property named <property>", 1, [](auto args) {
            auto property = args[0];
            if (Config::boolValues.find(property) != Config::boolValues.end()) {
                if (boolValues.at(property)) {
                    logger->Infof("true");
                } else {
                    logger->Infof("false");
                }
            } else if (Config::intValues.find(property) != Config::intValues.end()) {
                logger->Infof("%d", intValues.at(property));
            } else if (Config::stringValues.find(property) != Config::stringValues.end()) {
                logger->Infof("%s", stringValues.at(property).c_str());
            } else {
                logger->Errorf("Property '%s' not found", property.c_str());
            }
        });
    Console::RegisterCommand(cfgGetDefinition);
    CommandDefinition cfgSetDefinition(
        "cfg_set", "cfg_set <property> <value> - sets the property named <property> to <value>", 2, [](auto args) {
            auto property = args[0];
            auto value = args[1];
            if (boolValues.find(property) != boolValues.end()) {
                if (value != "false" && value != "true") {
                    logger->Errorf("Property '%s' is a bool, but given value '%s' is not a bool", property, value);
                    return;
                }
                boolValues.at(property) = value == "true";
            } else if (intValues.find(property) != intValues.end()) {
                auto convertedValue = std::strtol(value.c_str(), nullptr, 0);
                if (convertedValue == 0 && value != "0") {
                    logger->Errorf("Invalid int value %s for property '%s'", value.c_str(), property.c_str());
                }
                intValues.at(property) = convertedValue;
            } else if (stringValues.find(property) != stringValues.end()) {
                stringValues.at(property) = value;
            } else {
                logger->Errorf("Property '%s' not found", property.c_str());
            }
        });
    Console::RegisterCommand(cfgSetDefinition);
}

DynamicBoolProperty AddBool(std::string const & name, bool value)
{
    boolValues.insert_or_assign(name, value);
    return DynamicBoolProperty(value, name);
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
