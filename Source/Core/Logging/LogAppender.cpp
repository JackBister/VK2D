#include "LogAppender.h"

void LogAppender::Append(LogMessage message)
{
    if (message.GetLevel() < sharedMinLevel) {
        return;
    }
    auto name = message.GetLoggerName();
    if (minLevels.find(name) != minLevels.end() && message.GetLevel() < minLevels.at(name)) {
        return;
    }
    AppendImpl(message);
}

void LogAppender::SetMinimumLevel(LogLevel level)
{
    sharedMinLevel = level;
}

void LogAppender::SetMinimumLevel(std::string const & name, LogLevel level)
{
    minLevels[name] = level;
}
