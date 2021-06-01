#include "Logger.h"

#include <cstdarg>
#include <cstdint>
#include <vector>

#include "LogAppender.h"
#include "LogMessage.h"
#include "LoggerFactory.h"

Logger::Logger(std::string name, std::shared_ptr<LogAppender> appender) : appender(appender), name(name) {}

std::unique_ptr<Logger> Logger::Create(std::string const & name)
{
    return LoggerFactory::GetInstance()->CreateLogger(name);
}

void Logger::Tracef(char const * format, ...)
{
    va_list args;
    va_start(args, format);
    auto messageString = Sprintf(format, args);
    va_end(args);

    appender->Append(LogMessage(name, LogLevel::TRACE, messageString));
}

void Logger::Infof(char const * format, ...)
{
    va_list args;
    va_start(args, format);
    auto messageString = Sprintf(format, args);
    va_end(args);

    appender->Append(LogMessage(name, LogLevel::INFO, messageString));
}

void Logger::Warnf(char const * format, ...)
{
    va_list args;
    va_start(args, format);
    auto messageString = Sprintf(format, args);
    va_end(args);

    appender->Append(LogMessage(name, LogLevel::WARN, messageString));
}

void Logger::Errorf(char const * format, ...)
{
    va_list args;
    va_start(args, format);
    auto messageString = Sprintf(format, args);
    va_end(args);

    appender->Append(LogMessage(name, LogLevel::ERROR, messageString));
}

void Logger::Severef(char const * format, ...)
{
    va_list args;
    va_start(args, format);
    auto messageString = Sprintf(format, args);
    va_end(args);

    appender->Append(LogMessage(name, LogLevel::SEVERE, messageString));
}

std::string Logger::Sprintf(char const * format, va_list args)
{
    auto size = vsnprintf(nullptr, 0, format, args);
    std::vector<char> buf(size + 1);
    vsnprintf(buf.data(), buf.size(), format, args);
    return std::string(buf.data());
}
