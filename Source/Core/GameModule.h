#pragma once

#include <functional>
#include <glm/fwd.hpp>

#include "Core/DllExport.h"
#include "Core/FrameContext.h"
#include "Core/Queue.h"
#include "Core/Serialization/SerializedValue.h"

class CameraComponent;
class CommandBuffer;
class DeserializationContext;
class Entity;
class PhysicsWorld;
class RenderSystem;
class Scene;

namespace GameModule
{
enum class FrameStage {
    INPUT,
    TIME,
    PHYSICS,
    TICK,
    RENDER,
};

void AddEntity(Entity *);
void BeginPlay();
Entity * GetEntityByIdx(size_t idx);
Entity * GetEntityByName(std::string const &);
size_t GetEntityCount();
Entity * GetMainCamera();
// TODO:
PhysicsWorld * GetPhysicsWorld();
Scene * GetScene();
void Init();
void LoadDLL(std::string const &);
void LoadScene(std::string const &);
void OnFrameStart(std::function<void()>);
void RemoveEntity(Entity *);
SerializedObject SerializePhysics();
void SetPhysicsWorld(PhysicsWorld *);
void SetScene(Scene *);
void TakeCameraFocus(Entity *);
void Tick(FrameContext &);
void TickEntities();
void UnloadDLL(std::string const &);
void UnloadScene();
};
