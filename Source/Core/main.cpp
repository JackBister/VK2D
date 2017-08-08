#include <cstdio>
#include <thread>

#include "glm/glm.hpp"
#include "SDL/SDL.h"

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

//TODO:
std::string const kSceneFile = "Examples/FlappyPong/main.scene";

//TODO: ResourceManager race conditions - Renderer has access to resMan,
//but only uses it before main thread starts anyway so right now this is a non-issue.
void MainThread(ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue, Queue<RenderCommand>::Writer&& renderQueue,
				Semaphore * swapSync) noexcept
{
	FILE * f = fopen(kSceneFile.c_str(), "rb");
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

	Scene scene(kSceneFile, resMan, std::move(inputQueue), std::move(renderQueue), serializedScene);

	scene.time_.Start();
	scene.BroadcastEvent("BeginPlay");
	while (true) {
		swapSync->Wait();
		scene.Tick();
		//scene.EndFrame();
	}
}

#undef main
int main(int argc, char *argv[])
{
	using namespace std::literals::chrono_literals;
	//Sends input from GPU thread -> main thread
	Queue<SDL_Event> inputQueue;
	//Sends commands from main thread -> GPU thread
	Queue<RenderCommand> renderQueue;
	ResourceManager resMan(renderQueue.GetWriter());
	SDL_Init(SDL_INIT_EVERYTHING);
	Semaphore renderingFinishedSem;
	Renderer renderer(&resMan, renderQueue.GetReader(), &renderingFinishedSem, "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	auto mainThread = std::thread(MainThread, &resMan, inputQueue.GetReader(), renderQueue.GetWriter(), &renderingFinishedSem);
	auto inputWriter = inputQueue.GetWriter();

	for (int i = 0; i < 2; ++i) {
		renderingFinishedSem.Signal();
	}

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
