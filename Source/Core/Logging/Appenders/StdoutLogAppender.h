#pragma once

#include "Core/Logging/LogAppender.h"

class StdoutLogAppender : public LogAppender
{
protected:
    virtual void AppendImpl(LogMessage const & message) final override;
};
