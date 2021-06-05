#pragma once

#include <string>
#include <unordered_map>

#include "LogLevel.h"
#include "LogMessage.h"
#include "Util/DllExport.h"

class EAPI LogAppender
{
public:
    void Append(LogMessage message);

    void SetMinimumLevel(LogLevel);
    void SetMinimumLevel(std::string const & name, LogLevel);

protected:
    virtual void AppendImpl(LogMessage const & message) = 0;

private:
    std::unordered_map<std::string, LogLevel> minLevels;
    LogLevel sharedMinLevel = LogLevel::TRACE;
};
