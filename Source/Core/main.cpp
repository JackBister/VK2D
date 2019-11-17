#include <cstdio>
#include <thread>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Components/component.h"
#include "Core/Input.h"
#include "Core/Logging/Appenders/StdoutLogAppender.h"
#include "Core/Logging/Logger.h"
#include "Core/Logging/LoggerFactory.h"
#include "Core/Queue.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/RenderPrimitiveFactory.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/ResourceManager.h"
#include "Core/Semaphore.h"
#include "Core/SetThreadName.h"
#include "Core/Util/DefaultFileSlurper.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/scene.h"
#include "Core/transform.h"

static const auto logger = Logger::Create("main");

#undef main
int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("Usage: %s <scene file>\n", argv[0]);
        return 1;
    }
    std::string sceneFileName(argv[1]);
    ImGui::CreateContext();
    SDL_Init(SDL_INIT_EVERYTHING);
    Renderer renderer("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
    ResourceManager::Init(&renderer);
    RenderPrimitiveFactory(&renderer).CreatePrimitives();
    RenderSystem renderSystem(&renderer);

    SetThreadName(std::this_thread::get_id(), "Main Thread");

    GameModule::Init(&renderSystem);

    GameModule::LoadScene(sceneFileName);
    GameModule::BeginPlay();
    while (true) {
        char const * sdlErr = SDL_GetError();
        if (*sdlErr != '\0') {
            logger->Errorf("SDL_GetError returned an error when ticking: %s", sdlErr);
        }
        GameModule::Tick();
    }
    SDL_Quit();
    return renderer.abortCode;
}
