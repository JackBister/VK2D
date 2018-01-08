#include <cstdio>
#include <thread>

#include "glm/glm.hpp"
#include "SDL/SDL.h"

#include "Core/Components/CameraComponent.h"
#include "Core/Components/component.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Queue.h"
#include "Core/Rendering/RenderCommand.h"
#include "Core/Rendering/Renderer.h"
#include "Core/ResourceManager.h"
#include "Core/scene.h"
#include "Core/Semaphore.h"
#include "Core/sprite.h"
#include "Core/transform.h"

#if defined(_WIN64) && defined(_DEBUG)
#include "Windows.h"
#endif

#ifdef _DEBUG
void SetThreadName(std::thread::id id, char const * name)
{
#ifdef _WIN64
#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)

	const DWORD MS_VC_EXCEPTION = 0x406D1388;
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = *(DWORD *)&id;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	} __except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
}
#endif

//TODO: ResourceManager race conditions - Renderer has access to resMan,
//but only uses it before main thread starts anyway so right now this is a non-issue.
void MainThread(ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue, Queue<RenderCommand>::Writer&& renderQueue, std::string sceneFileName, Renderer * renderer) noexcept
{
	FILE * f = fopen(sceneFileName.c_str(), "rb");
	std::string serializedScene;
	if (f) {
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		fseek(f, 0, SEEK_SET);
		std::vector<char> buf(length + 1);
		fread(&buf[0], 1, length, f);
		serializedScene = std::string(buf.begin(), buf.end());
	} else {
		renderQueue.Push(RenderCommand(RenderCommand::AbortParams(1)));
		return;
	}

	Scene scene(sceneFileName, resMan, std::move(inputQueue), std::move(renderQueue), serializedScene, renderer);

	scene.time_.Start();
	scene.BroadcastEvent("BeginPlay");
	while (true) {
		scene.Tick();
		//scene.EndFrame();
	}
}

#undef main
int main(int argc, char *argv[])
{
	using namespace std::literals::chrono_literals;
	if (argc < 2) {
		printf("Usage: %s <scene file>\n", argv[0]);
		return 1;
	}
	std::string sceneFileName(argv[1]);
	//Sends input from GPU thread -> main thread
	Queue<SDL_Event> inputQueue;
	//Sends commands from main thread -> GPU thread
	Queue<RenderCommand> renderQueue;
	ResourceManager resMan(renderQueue.GetWriter());
	SDL_Init(SDL_INIT_EVERYTHING);
	Renderer renderer(&resMan, /* renderQueue.GetReader(),*/ "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	auto mainThread = std::thread(MainThread, &resMan, inputQueue.GetReader(), renderQueue.GetWriter(), sceneFileName, &renderer);
	auto inputWriter = inputQueue.GetWriter();

	SetThreadName(std::this_thread::get_id(), "Rendering Thread");
	SetThreadName(mainThread.get_id(), "Main Thread");
	
	while (!renderer.isAborting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			inputWriter.Push(std::move(e));
		}
		char const * sdlErr = SDL_GetError();
		if (*sdlErr != '\0') {
			printf("%s\n", sdlErr);
		}
		renderer.DrainQueue();
	}
	mainThread.join();
	SDL_Quit();
	return renderer.abortCode;
}
