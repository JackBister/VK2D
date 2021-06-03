#pragma once

#include <functional>
#include <glm/fwd.hpp>

#include "Core/FrameContext.h"
#include "Serialization/SerializedValue.h"
#include "Util/DllExport.h"
#include "Util/Queue.h"

class CameraComponent;
class CommandBuffer;
class DeserializationContext;
class Entity;
class RenderSystem;

namespace GameModule
{
enum class FrameStage {
    INPUT,
    TIME,
    PHYSICS,
    TICK,
    RENDER,
};

void BeginPlay();
void Init();
void OnFrameStart(std::function<void()>);
void Tick(FrameContext &);
void TickEntities();
};
