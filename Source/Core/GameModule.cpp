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
#include "Jobs/JobEngine.h"

static const auto logger = Logger::Create("GameModule");

namespace GameModule
{
Semaphore renderLock;
FrameStage currFrameStage;
std::vector<Entity *> entities;
Entity * mainCameraEntity;
CameraComponent * mainCameraComponent;
PhysicsWorld * physicsWorld;
RenderSystem * renderSystem;
Scene * scene = nullptr;
UiRenderSystem * uiRenderSystem;

std::vector<std::function<void()>> onFrameStart;

void AddEntity(Entity * e)
{
    entities.push_back(e);
    e->FireEvent("BeginPlay");
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

void Init()
{
    renderLock.Signal();
    renderSystem = RenderSystem::GetInstance();
    uiRenderSystem = UiRenderSystem::GetInstance();

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

void SetPhysicsWorld(PhysicsWorld * pw)
{
    if (physicsWorld == nullptr) {
        physicsWorld = pw;
        return;
    }
    auto grav = pw->GetGravity();
    physicsWorld->SetGravity(grav);
}

void SetScene(Scene * s)
{
    scene = s;
}

void TakeCameraFocus(Entity * camera)
{
    mainCameraEntity = camera;
    mainCameraComponent = (CameraComponent *)camera->GetComponent("CameraComponent");
}

void Tick(FrameContext & context)
{
    OPTICK_EVENT();
    OPTICK_TAG("FrameNumber", context.frameNumber)
    currFrameStage = FrameStage::INPUT;
    Input::Frame();
    currFrameStage = FrameStage::TIME;
    Time::Frame();

    DebugDrawSystem::GetInstance()->Tick();

    if (Input::GetKeyDown(KC_F1)) {
        Console::ToggleVisible();
    }

    uiRenderSystem->StartFrame();

    for (auto fun : onFrameStart) {
        fun();
    }

    onFrameStart.clear();

    currFrameStage = FrameStage::PHYSICS;
    // TODO: substeps
    physicsWorld->Tick(Time::GetDeltaTime());
    currFrameStage = FrameStage::TICK;
    TickEntities();

    EditorSystem::OnGui();
    Console::OnGui();

    uiRenderSystem->EndFrame(context);

    PreRenderCommands preRenderCommands = []() {
        OPTICK_EVENT("GatherPreRendercommands");
        PreRenderCommands::Builder builder;
        for (auto entity : entities) {
            entity->FireEvent("PreRender", {{"commandBuilder", &builder}});
        }
        return builder.Build();
    }();

    currFrameStage = FrameStage::RENDER;

    // TODO: I feel like I must be doing something wrong here. But it does accomplish what I want, which is to have CPU
    // frame N and GPU frame N-1 processing concurrently.
    renderLock.Wait();
    auto jobEngine = JobEngine::GetInstance();
    auto renderJob = jobEngine->CreateJob({}, [context, preRenderCommands]() {
        OPTICK_EVENT("GpuTick")
        OPTICK_TAG("FrameNumber", context.frameNumber)
        auto ctx = context;
        renderSystem->StartFrame(ctx, preRenderCommands);
        renderSystem->PreRenderFrame(ctx, preRenderCommands);
        renderSystem->RenderFrame(ctx);
        renderLock.Signal();
        FrameContext::Destroy(ctx);
    });
    jobEngine->ScheduleJob(renderJob, JobPriority::HIGH);
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
