#include "StdoutLogAppender.h"

void StdoutLogAppender::Append(LogMessage message) const
{
    auto levelString = ToString(message.GetLevel());
    printf("%s - [%s] %s\n", message.GetLoggerName().c_str(), levelString.c_str(), message.GetMessage().c_str());
}
