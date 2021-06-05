#include "LoggerFactory.h"

#include "LogAppender.h"
#include "Logger.h"

#include "Appenders/CompositeAppender.h"
#include "Appenders/StdoutLogAppender.h"
#include "Console/Console.h"

LoggerFactory::LoggerFactory(std::shared_ptr<LogAppender> appender) : appender(appender) {}

LoggerFactory * LoggerFactory::GetInstance()
{
    static LoggerFactory * singleton = nullptr;
    if (singleton == nullptr) {
        // TODO: Find a better place to configure this
        auto stdOutAppender = std::make_shared<StdoutLogAppender>();
        auto consoleAppender = Console::GetAppender();
        consoleAppender->SetMinimumLevel("Vulkan", LogLevel::WARN);
        std::vector<std::shared_ptr<LogAppender>> appenders = {stdOutAppender, consoleAppender};
        auto compositeAppender = std::make_shared<CompositeAppender>(appenders);
        singleton = new LoggerFactory(compositeAppender);
    }
    return singleton;
}

Logger LoggerFactory::CreateLogger(std::string name)
{
    return Logger(name, appender);
}
