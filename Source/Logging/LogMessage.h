#pragma once

#include <cstdint>
#include <string>

#include "LogLevel.h"

class LogMessage
{
public:
    LogMessage(std::string loggerName, LogLevel level, std::string message)
        : loggerName(loggerName), level(level), message(message)
    {
    }

    inline std::string GetLoggerName() const { return loggerName; }
    inline LogLevel GetLevel() const { return level; }
    inline std::string GetMessage() const { return message; }

private:
    std::string loggerName;
    LogLevel level;
    std::string message;
};
