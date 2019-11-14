#include "LoggerFactory.h"

#include "LogAppender.h"
#include "Logger.h"

#include "Core/Logging/Appenders/StdoutLogAppender.h"

LoggerFactory::LoggerFactory(std::shared_ptr<LogAppender> appender) : appender(appender) {}

LoggerFactory * LoggerFactory::GetInstance()
{
    static LoggerFactory * singleton = nullptr;
    if (singleton == nullptr) {
		// TODO: Find a better place to configure this
        singleton = new LoggerFactory(std::make_shared<StdoutLogAppender>());
	}
    return singleton;
}

std::unique_ptr<Logger> LoggerFactory::CreateLogger(std::string name)
{
    return std::make_unique<Logger>(name, appender);
}
