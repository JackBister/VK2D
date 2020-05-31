#include "GameModule.h"

#include <vector>

#include <optick/optick.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Console/Console.h"
#include "Core/Input/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/DebugDrawSystem.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/Scene.h"
#include "Core/UI/EditorSystem.h"
#include "Core/dtime.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"

static const auto logger = Logger::Create("GameModule");

namespace GameModule
{
FrameStage currFrameStage;
std::vector<Entity *> entities;
Entity * mainCameraEntity;
CameraComponent * mainCameraComponent;
PhysicsWorld * physicsWorld;
RenderSystem * renderSystem;
Scene * scene = nullptr;

std::vector<std::function<void()>> onFrameStart;

void AddEntity(Entity * e)
{
    entities.push_back(e);
    e->FireEvent("BeginPlay");
}

void DeserializePhysics(DeserializationContext * deserializationContext, SerializedObject const & obj)
{
    if (physicsWorld == nullptr) {
        physicsWorld = (PhysicsWorld *)Deserializable::Deserialize(deserializationContext, obj);
        return;
    }
    auto grav = obj.GetObject("gravity").value();
    physicsWorld->SetGravity(
        glm::vec3(grav.GetNumber("x").value(), grav.GetNumber("y").value(), grav.GetNumber("z").value()));
}

Entity * GetEntityByIdx(size_t idx)
{
    if (entities.size() <= idx) {
        return nullptr;
    }
    return entities[idx];
}

Entity * GetEntityByName(std::string const & name)
{
    for (auto entity : entities) {
        if (entity->GetName() == name) {
            return entity;
        }
    }
    return nullptr;
}

size_t GetEntityCount()
{
    return entities.size();
}

Entity * GetMainCamera()
{
    return mainCameraEntity;
}

PhysicsWorld * GetPhysicsWorld()
{
    return physicsWorld;
}

Scene * GetScene()
{
    return scene;
}

void Init(RenderSystem * inRenderSystem)
{
    // TODO: This is dumb
    renderSystem = inRenderSystem;

    Console::Init(renderSystem);
    Input::Init();
    Time::Start();
    EditorSystem::Init();

    CommandDefinition dumpSceneCommand(
        "scene_dump", "scene_dump <to_file> - Dumps the scene into <to_file>", 1, [](auto args) {
            auto toFile = args[0];
            scene->SerializeToFile(toFile);
        });
    Console::RegisterCommand(dumpSceneCommand);

    CommandDefinition loadSceneCommand(
        "scene_load", "scene_load <filename> - Loads the scene defined in the given file.", 1, [](auto args) {
            auto fileName = args[0];
            LoadScene(fileName);
        });
    Console::RegisterCommand(loadSceneCommand);

    CommandDefinition unloadSceneCommand(
        "scene_unload", "scene_unload - Unloads the scene.", 0, [](auto args) { scene->Unload(); });
    Console::RegisterCommand(unloadSceneCommand);
}

void LoadScene(std::string const & fileName)
{
    if (scene) {
        scene->Unload();
    }
    scene = Scene::FromFile(fileName);
}

void OnFrameStart(std::function<void()> fun)
{
    onFrameStart.push_back(fun);
}

void PreRender()
{
    OPTICK_EVENT();
    PreRenderCommands::Builder builder;
    for (auto entity : entities) {
        entity->FireEvent("PreRender", {{"commandBuilder", &builder}});
    }
    renderSystem->PreRenderFrame(builder.Build());
}

void RemoveEntity(Entity * entity)
{
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        if (*it == entity) {
            entities.erase(it);
            return;
        }
    }
}

SerializedObject SerializePhysics()
{
    return physicsWorld->Serialize();
}

void TakeCameraFocus(Entity * camera)
{
    mainCameraEntity = camera;
    mainCameraComponent = (CameraComponent *)camera->GetComponent("CameraComponent");
}

void Tick()
{
    OPTICK_EVENT();
    currFrameStage = FrameStage::INPUT;
    Input::Frame();
    currFrameStage = FrameStage::TIME;
    Time::Frame();

    DebugDrawSystem::GetInstance()->Tick();

    // TODO: remove these (hardcoded serialize + dump keybinds)
#if 0
    if (Input::GetKeyDown(KC_F8)) {
        scenes[0].SerializeToFile("_dump.scene");
    }
    if (Input::GetKeyDown(KC_F9)) {
        scenes[0].LoadFile("main.scene");
        scenes[0].BroadcastEvent("BeginPlay");
    }
    if (Input::GetKeyDown(KC_F10)) {
        for (auto & fi : frameInfo) {
            fi.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
            fi.preRenderPassCommandBuffer->Reset();
            fi.preRenderPassCommandBuffer->BeginRecording(nullptr);
            fi.preRenderPassCommandBuffer->EndRecording();
            renderer->ExecuteCommandBuffer(fi.preRenderPassCommandBuffer, {}, {}, fi.canStartFrame);
        }
        mainCameraComponent = nullptr;
        mainCameraEntity = nullptr;
        scenes[0].Unload();
    }
#endif

    if (Input::GetKeyDown(KC_F1)) {
        Console::ToggleVisible();
    }

    currFrameStage = FrameStage::FENCE_WAIT;
    renderSystem->StartFrame();

    for (auto fun : onFrameStart) {
        fun();
    }

    onFrameStart.clear();

    currFrameStage = FrameStage::PHYSICS;
    // TODO: substeps
    physicsWorld->world->stepSimulation(Time::GetDeltaTime());
    currFrameStage = FrameStage::TICK;
    TickEntities();

    EditorSystem::OnGui();
    Console::OnGui();

    PreRender();

    currFrameStage = FrameStage::RENDER;
    renderSystem->RenderFrame();
}

void TickEntities()
{
    OPTICK_EVENT();
    for (auto entity : entities) {
        entity->FireEvent("Tick", {{"deltaTime", Time::GetDeltaTime()}});
    }
}

void UnloadScene()
{
    OPTICK_EVENT();

    scene->Unload();
    scene = nullptr;
}
};
