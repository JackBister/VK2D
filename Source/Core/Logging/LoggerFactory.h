#pragma once

#include <memory>
#include <string>

class LogAppender;
class Logger;

class LoggerFactory
{
public:
    LoggerFactory(std::shared_ptr<LogAppender> appender);

	static LoggerFactory * GetInstance();

    std::unique_ptr<Logger> CreateLogger(std::string name);

private:
    std::shared_ptr<LogAppender> appender;
};