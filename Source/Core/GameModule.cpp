#include "GameModule.h"

#include <vector>

#include <optick/optick.h>

#include "Console/Console.h"
#include "Core/Components/CameraComponent.h"
#include "Core/EntityManager.h"
#include "Core/Input/Input.h"
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
#include "Logging/Logger.h"

static const auto logger = Logger::Create("GameModule");

namespace GameModule
{
Semaphore renderLock;
FrameStage currFrameStage;
EntityManager * entityManager;
CameraComponent * mainCameraComponent;
PhysicsWorld * physicsWorld;
RenderSystem * renderSystem;
UiRenderSystem * uiRenderSystem;

std::vector<std::function<void()>> onFrameStart;

void Init()
{
    renderLock.Signal();
    entityManager = EntityManager::GetInstance();
    physicsWorld = PhysicsWorld::GetInstance();
    renderSystem = RenderSystem::GetInstance();
    uiRenderSystem = UiRenderSystem::GetInstance();

    Console::Init(renderSystem);
    Input::Init();
    Time::Start();
    EditorSystem::Init();
}

void OnFrameStart(std::function<void()> fun)
{
    onFrameStart.push_back(fun);
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
    auto res = renderSystem->GetResolution();
    Console::OnGui(res.x, res.y);

    uiRenderSystem->EndFrame(context);

    PreRenderCommands preRenderCommands = []() {
        OPTICK_EVENT("GatherPreRendercommands");
        PreRenderCommands::Builder builder;
        entityManager->BroadcastEvent("PreRender", {{"commandBuilder", &builder}});
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
    entityManager->BroadcastEvent("Tick", {{"deltaTime", Time::GetDeltaTime()}});
}
};
