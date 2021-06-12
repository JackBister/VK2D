#pragma once

#include <functional>

struct FrameContext;

namespace GameModule
{
void BeginPlay();
void Init();
void OnFrameStart(std::function<void()>);
void Tick(FrameContext &);
void TickEntities();
};
