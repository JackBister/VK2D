#include "GameModule.h"

#include <vector>

#include <optick/optick.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Console/Console.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/SubmittedCamera.h"
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
std::vector<Scene *> scenes;

std::vector<std::function<void()>> onFrameStart;

// TODO: Remove
std::vector<SubmittedCamera> submittedCameras;
std::vector<SubmittedMesh> submittedMeshes;
std::vector<SubmittedSprite> submittedSprites;

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

    CommandDefinition dumpSceneCommand(
        "scene_dump",
        "scene_dump <from_file> <to_file> - Dumps the scene load from <from_file> into <to_file>",
        2,
        [](auto args) {
            auto fromFile = args[0];
            auto toFile = args[1];

            for (auto scene : scenes) {
                if (scene->GetFileName() == fromFile) {
                    scene->SerializeToFile(toFile);
                    return;
                }
            }
            logger->Warnf("No scene with name '%s' found", fromFile.c_str());
        });
    Console::RegisterCommand(dumpSceneCommand);

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
                    break;
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

SerializedObject SerializePhysics()
{
    return physicsWorld->Serialize();
}

// TODO: Remove
void SubmitCamera(SubmittedCamera const & camera)
{
    submittedCameras.push_back(camera);
}

void SubmitMesh(SubmittedMesh const & mesh)
{
    submittedMeshes.push_back(mesh);
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
    OPTICK_EVENT();
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
    TickEntities();

    EditorSystem::OnGui();
    Console::OnGui();

    currFrameStage = FrameStage::RENDER;
    renderSystem->RenderFrame({submittedCameras, submittedMeshes, submittedSprites});

    submittedCameras.clear();
    submittedMeshes.clear();
    submittedSprites.clear();
}

void TickEntities()
{
    OPTICK_EVENT();
    for (auto entity : entities) {
        entity->FireEvent("Tick", {{"deltaTime", Time::GetDeltaTime()}});
    }
}
};
