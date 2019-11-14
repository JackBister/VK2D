#pragma once

#include "LogMessage.h"

class LogAppender
{
public:
    virtual void Append(LogMessage message) const = 0;
};
