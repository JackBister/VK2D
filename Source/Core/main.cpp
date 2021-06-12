#include <cstdio>
#include <thread>

#include <ThirdParty/SDL2/include/SDL.h>
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/optick/src/optick.h>

#include "Core/Config/Config.h"
#include "Core/FrameContext.h"
#include "Core/GameModule.h"
#include "Core/ProjectManager.h"
#include "Core/Rendering/BufferAllocator.h"
#include "Core/Rendering/RenderPrimitiveFactory.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/Rendering/ShaderProgramFactory.h"
#include "Core/Resources/Project.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/UI/EditorSystem.h"
#include "Jobs/JobEngine.h"
#include "Logging/Logger.h"
#include "RenderingBackend/Renderer.h"
#include "Util/SetThreadName.h"

static const auto logger = Logger::Create("main");

#undef main
int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("Usage: %s <project file>\n", argv[0]);
        return 1;
    }

    int filenameIndex = 0;
    bool startInEditor = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-editor") == 0) {
            startInEditor = true;
        } else {
            filenameIndex = i;
        }
    }
    SetThreadName(std::this_thread::get_id(), "Main Thread");
    OPTICK_THREAD("Main Thread");

    std::optional<std::string> projectFileName;
    if (filenameIndex != 0) {
        projectFileName = argv[filenameIndex];
    } else {
        startInEditor = true;
    }
#if _DEBUG
    srand(0);
#else
    srand(time(0));
#endif
    ImGui::CreateContext();
    SDL_Init(SDL_INIT_EVERYTHING);
    Config::Init();
    int numJobThreads = 5;
    JobEngine jobEngine(numJobThreads);
    jobEngine.RegisterMainThread();
    RendererConfig cfg;
    cfg.windowResolution.x = 1280;
    cfg.windowResolution.y = 720;
    cfg.presentMode = PresentMode::FIFO;
    Renderer renderer("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, cfg, numJobThreads + 1);
    RenderPrimitiveFactory renderPrimitiveFactory(&renderer);
    renderPrimitiveFactory.CreatePrimitives();
    RenderSystem renderSystem(&renderer);
    BufferAllocator bufferAllocator(&renderSystem);
    ResourceManager::Init(&renderSystem);
    renderPrimitiveFactory.LateCreatePrimitives();
    ShaderProgramFactory::CreateResources();
    renderSystem.Init();

    GameModule::Init();

    auto projectManager = ProjectManager::GetInstance();

    if (projectFileName.has_value()) {
        if (!projectManager->ChangeProject(projectFileName.value())) {
            logger.Severe("Failed to load initial project with name={}", projectFileName.value());
            return 1000;
        }
    }

    if (startInEditor) {
        EditorSystem::OpenEditor();
    }

    uint64_t frameNumber = 0;
    uint32_t currentGpuFrameIndex = 0;
    while (true) {
        OPTICK_FRAME("MainThread");
        char const * sdlErr = SDL_GetError();
        if (*sdlErr != '\0') {
            logger.Error("SDL_GetError returned an error when ticking: {}", sdlErr);
            SDL_ClearError();
        }
        FrameContext context = {};
        context.frameNumber = frameNumber;
        context.currentGpuFrameIndex = currentGpuFrameIndex;
        GameModule::Tick(context);
        currentGpuFrameIndex = context.currentGpuFrameIndex;
        frameNumber++;
    }
    SDL_Quit();
    return renderer.abortCode;
}
