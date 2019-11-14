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
        auto appender = std::make_shared<StdoutLogAppender>();
        appender->SetMinimumLevel("Vulkan", LogLevel::WARN);
        singleton = new LoggerFactory(appender);
    }
    return singleton;
}

std::unique_ptr<Logger> LoggerFactory::CreateLogger(std::string name)
{
    return std::make_unique<Logger>(name, appender);
}
