#include "Logger.h"

#include "LoggerFactory.h"

Logger::Logger(std::string name, std::shared_ptr<LogAppender> appender) : appender(appender), name(name) {}

Logger Logger::Create(std::string const & name)
{
    return LoggerFactory::GetInstance()->CreateLogger(name);
}
