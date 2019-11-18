﻿#include "GameModule.h"

#include <nlohmann/json.hpp>
#include <vector>

#include "Core/Components/CameraComponent.h"
#include "Core/Console/Console.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/UI/EditorSystem.h"
#include "Core/dtime.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/Resources/Scene.h"

static const auto logger = Logger::Create("GameModule");

namespace GameModule
{
FrameStage currFrameStage;
std::vector<Entity *> entities;
Entity * mainCameraEntity;
CameraComponent * mainCameraComponent;
PhysicsWorld * physicsWorld;
RenderSystem * renderSystem;
std::vector<Scene *> scenes;

std::vector<std::function<void()>> onFrameStart;

// TODO: Remove
std::vector<SubmittedCamera> submittedCameras;
std::vector<SubmittedSprite> submittedSprites;

void AddEntity(Entity * e)
{
    entities.push_back(e);
    e->FireEvent("BeginPlay");
}

void CreateResources(std::function<void(ResourceCreationContext &)> fun)
{
    ResourceManager::CreateResources(fun);
}

void DeserializePhysics(std::string const & str)
{
    if (physicsWorld == nullptr) {
        physicsWorld = (PhysicsWorld *)Deserializable::DeserializeString(str);
        return;
    }
    auto j = nlohmann::json::parse(str);
    physicsWorld->SetGravity(glm::vec3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
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
        if (entity->name == name) {
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

void Init(RenderSystem * inRenderSystem)
{
    // TODO: This is dumb
    renderSystem = inRenderSystem;

    Console::Init(renderSystem);
    Input::Init();
    Time::Start();

    CommandDefinition loadSceneCommand(
        "scene_load", "scene_load <filename> - Loads the scene defined in the given file.", 1, [](auto args) {
            auto fileName = args[0];
            LoadScene(fileName);
        });
    Console::RegisterCommand(loadSceneCommand);

    CommandDefinition unloadSceneCommand(
        "scene_unload", "scene_unload <filename> - Unloads the scene defined in the given file.", 1, [](auto args) {
            auto fileName = args[0];
            for (auto scene : scenes) {
                if (scene->GetFileName() == fileName) {
                    scene->Unload();
                }
            }
            scenes.erase(std::remove_if(scenes.begin(),
                                        scenes.end(),
                                        [fileName](Scene * scene) { return scene->GetFileName() == fileName; }),
                         scenes.end());
        });
    Console::RegisterCommand(unloadSceneCommand);
}

void LoadScene(std::string const & fileName)
{
    scenes.push_back(Scene::FromFile(fileName));
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

std::string SerializePhysics()
{
    return physicsWorld->Serialize();
}

// TODO: Remove
void SubmitCamera(SubmittedCamera const & camera)
{
    submittedCameras.push_back(camera);
}

// TODO: Remove
void SubmitSprite(SubmittedSprite const & sprite)
{
    submittedSprites.push_back(sprite);
}

void TakeCameraFocus(Entity * camera)
{
    mainCameraEntity = camera;
    mainCameraComponent = (CameraComponent *)camera->GetComponent("CameraComponent");
}

void Tick()
{
    currFrameStage = FrameStage::INPUT;
    Input::Frame();
    currFrameStage = FrameStage::TIME;
    Time::Frame();

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
    for (auto entity : entities) {
        entity->FireEvent("Tick", {{"deltaTime", Time::GetDeltaTime()}});
    }

    EditorSystem::OnGui();
    Console::OnGui();

    currFrameStage = FrameStage::RENDER;
    renderSystem->RenderFrame({submittedCameras, submittedSprites});

    submittedCameras.clear();
    submittedSprites.clear();
}
};
