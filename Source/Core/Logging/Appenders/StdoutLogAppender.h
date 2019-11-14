#pragma once

#include "Core/Logging/LogAppender.h"

class StdoutLogAppender : public LogAppender
{
    virtual void Append(LogMessage message) const final override;
};
