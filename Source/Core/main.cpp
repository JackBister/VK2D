#include <cstdio>
#include <thread>

#include "glm/glm.hpp"
#include "SDL2/SDL.h"

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

#undef main
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <scene file>\n", argv[0]);
		return 1;
	}
	std::string sceneFileName(argv[1]);
	Queue<SDL_Event> inputQueue;
	SDL_Init(SDL_INIT_EVERYTHING);
	Renderer renderer("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	ResourceManager resMan(&renderer);
	auto inputWriter = inputQueue.GetWriter();

#ifdef _DEBUG
	SetThreadName(std::this_thread::get_id(), "Main Thread");
#endif

	GameModule::Init(&resMan, inputQueue.GetReader(), &renderer);

	GameModule::LoadScene(sceneFileName);

	GameModule::BeginPlay();
	while (true) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			inputWriter.Push(std::move(e));
		}
		char const * sdlErr = SDL_GetError();
		if (*sdlErr != '\0') {
			printf("%s\n", sdlErr);
		}
		GameModule::Tick();
	}
	SDL_Quit();
	return renderer.abortCode;
}
