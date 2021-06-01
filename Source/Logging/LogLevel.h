#pragma once

#include <cassert>
#include <string>

enum class LogLevel { TRACE, INFO, WARN, ERROR, SEVERE };

std::string ToString(LogLevel level);
