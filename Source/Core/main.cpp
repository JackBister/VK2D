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
#include "Core/Rendering/Shader.h"
#include "Core/Rendering/ViewDef.h"
#include "Core/ResourceManager.h"
#include "Core/scene.h"
#include "Core/sprite.h"
#include "Core/transform.h"

//TODO:
const std::string sceneFile = "Examples/FlappyPong/main.scene";

//TODO: ResourceManager race conditions - Renderer has access to resMan,
//but only uses it before main thread starts anyway so right now this is a non-issue.
void MainThread(ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue, Queue<RenderCommand>::Writer&& renderQueue,
				Queue<ViewDef *>::Reader&& viewDefQueue) noexcept
{
	FILE * f = fopen(sceneFile.c_str(), "rb");
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
	}

	Scene scene(sceneFile, resMan, std::move(inputQueue), std::move(renderQueue), std::move(viewDefQueue), serializedScene);

	scene.time.Start();
	scene.BroadcastEvent("BeginPlay");
	while (true) {
		scene.Tick();
		scene.EndFrame();
	}
}

#undef main
int main(int argc, char *argv[])
{
	//Sends input from GPU thread -> main thread
	Queue<SDL_Event> inputQueue;
	//Sends commands from main thread -> GPU thread
	Queue<RenderCommand> renderQueue;
	//Sends ViewDefs available for the scene to use from GPU thread -> main thread
	Queue<ViewDef *> viewDefQueue;
	ResourceManager * resMan = new ResourceManager(renderQueue.GetWriter());
	SDL_Init(SDL_INIT_EVERYTHING);
	Renderer renderer(resMan, renderQueue.GetReader(), viewDefQueue.GetWriter(), "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	auto mainThread = std::thread(MainThread, resMan, inputQueue.GetReader(), renderQueue.GetWriter(), viewDefQueue.GetReader());
	auto inputWriter = inputQueue.GetWriter();

	while (!renderer.isAborting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			inputWriter.Push(e);
			//printf("pushing\n");
		}
		const char * sdlErr = SDL_GetError();
		if (*sdlErr != '\0') {
			printf("%s\n", sdlErr);
		}
		renderer.DrainQueue();
	}
	mainThread.join();
	SDL_Quit();
	return renderer.abortCode;
}
