#pragma once

#include <functional>
#include <glm/fwd.hpp>

#include "Core/DllExport.h"
#include "Core/Queue.h"
#include "Core/Serialization/SerializedValue.h"

class CameraComponent;
class CommandBuffer;
class DeserializationContext;
class Entity;
class PhysicsWorld;
class RenderSystem;
struct SubmittedCamera;
struct SubmittedMesh;
struct SubmittedSprite;

namespace GameModule
{
enum class FrameStage {
    INPUT,
    TIME,
    FENCE_WAIT,
    PHYSICS,
    TICK,
    RENDER,
};

void AddEntity(Entity *);
void BeginPlay();
void DeserializePhysics(DeserializationContext * deserializationContext, SerializedObject const &);
Entity * GetEntityByIdx(size_t idx);
Entity * GetEntityByName(std::string const &);
size_t GetEntityCount();
Entity * GetMainCamera();
// TODO:
PhysicsWorld * GetPhysicsWorld();
void Init(RenderSystem * renderSystem);
void LoadDLL(std::string const &);
void LoadScene(std::string const &);
void OnFrameStart(std::function<void()>);
void PreRender();
void RemoveEntity(Entity *);
SerializedObject SerializePhysics();
void SubmitCamera(SubmittedCamera const & camera);
void SubmitMesh(SubmittedMesh const & mesh);
void SubmitSprite(SubmittedSprite const & sprite);
void TakeCameraFocus(Entity *);
void Tick();
void TickEntities();
void UnloadDLL(std::string const &);
};
