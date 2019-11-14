#include "LogLevel.h"

std::string ToString(LogLevel level) {
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::SEVERE:
        return "SEVERE";
    default:
        assert(false);
        return "UNKNOWN_LOG_LEVEL";
    }
}
