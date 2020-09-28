#include "StdoutLogAppender.h"

void StdoutLogAppender::AppendImpl(LogMessage const & message)
{
    auto levelString = ToString(message.GetLevel());
    printf("%s - [%s] %s\n", message.GetLoggerName().c_str(), levelString.c_str(), message.GetMessage().c_str());
}
