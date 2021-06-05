#pragma once

#include <cassert>
#include <string>

#undef TRACE
#undef INFO
#undef WARN
#undef ERROR
#undef SEVERE

enum class LogLevel { TRACE, INFO, WARN, ERROR, SEVERE };

std::string ToString(LogLevel level);
