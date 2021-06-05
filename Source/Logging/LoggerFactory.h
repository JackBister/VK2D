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

    Logger CreateLogger(std::string name);

private:
    std::shared_ptr<LogAppender> appender;
};