#pragma once

#include <memory>

#include "CommandDefinition.h"

class LogAppender;
class RenderSystem;

namespace Console
{
std::shared_ptr<LogAppender> GetAppender();
void Init(RenderSystem *);
void OnGui();
void RegisterCommand(CommandDefinition const &);
void ToggleVisible();
};
