#pragma once

#include <string>
#include <unordered_map>

#include "LogLevel.h"
#include "LogMessage.h"

class LogAppender
{
public:
    void Append(LogMessage message) const;

    void SetMinimumLevel(LogLevel);
    void SetMinimumLevel(std::string const & name, LogLevel);

protected:
    virtual void AppendImpl(LogMessage const & message) const = 0;

private:
    std::unordered_map<std::string, LogLevel> minLevels;
    LogLevel sharedMinLevel = LogLevel::TRACE;
};
