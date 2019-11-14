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

	inline std::string GetLoggerName() { return loggerName; }
    inline LogLevel GetLevel() { return level; }
    inline std::string GetMessage() { return message; }

private:
    std::string loggerName;
    LogLevel level;
    std::string message;
};
