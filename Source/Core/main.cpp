#include <cstdio>
#include <thread>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <optick/optick.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Components/component.h"
#include "Core/Config/Config.h"
#include "Core/GameModule.h"
#include "Core/Input.h"
#include "Core/Logging/Appenders/StdoutLogAppender.h"
#include "Core/Logging/Logger.h"
#include "Core/Logging/LoggerFactory.h"
#include "Core/Queue.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/BufferAllocator.h"
#include "Core/Rendering/RenderPrimitiveFactory.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/ShaderProgramFactory.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"
#include "Core/UI/EditorSystem.h"
#include "Core/Util/DefaultFileSlurper.h"
#include "Core/Util/SetThreadName.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/transform.h"

static const auto logger = Logger::Create("main");

#undef main
int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("Usage: %s <scene file>\n", argv[0]);
        return 1;
    }

    bool startInEditor = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-editor") == 0) {
            startInEditor = true;
        }
    }

    std::string sceneFileName(argv[argc - 1]);
    ImGui::CreateContext();
    SDL_Init(SDL_INIT_EVERYTHING);
    Config::Init();
    RendererConfig cfg;
    cfg.windowResolution.x = 1280;
    cfg.windowResolution.y = 720;
    cfg.presentMode = PresentMode::FIFO;
    Renderer renderer("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, cfg);
    RenderPrimitiveFactory renderPrimitiveFactory(&renderer);
    renderPrimitiveFactory.CreatePrimitives();
    RenderSystem renderSystem(&renderer);
    BufferAllocator bufferAllocator(&renderSystem);
    ResourceManager::Init(&renderSystem);
    renderPrimitiveFactory.LateCreatePrimitives();
    ShaderProgramFactory::CreateResources();
    renderSystem.Init();

    SetThreadName(std::this_thread::get_id(), "Main Thread");
    OPTICK_THREAD("Main Thread");

    GameModule::Init(&renderSystem);

    GameModule::LoadScene(sceneFileName);

    if (startInEditor) {
        EditorSystem::OpenEditor();
    }

    while (true) {
        OPTICK_FRAME("MainThread");
        char const * sdlErr = SDL_GetError();
        if (*sdlErr != '\0') {
            logger->Errorf("SDL_GetError returned an error when ticking: %s", sdlErr);
        }
        GameModule::Tick();
    }
    SDL_Quit();
    return renderer.abortCode;
}
