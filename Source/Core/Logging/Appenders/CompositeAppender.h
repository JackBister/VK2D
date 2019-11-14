#pragma once

#include <memory>
#include <vector>

#include "Core/Logging/LogAppender.h"

class CompositeAppender : public LogAppender
{
public:
    CompositeAppender(std::vector<std::shared_ptr<LogAppender>> appenders);

protected:
    virtual void AppendImpl(LogMessage const & message) const final override;

private:
    std::vector<std::shared_ptr<LogAppender>> appenders;
};
