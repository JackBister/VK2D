#include <cstdio>

#include "glm/glm.hpp"
#include "SDL/SDL.h"

#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/render.h"
#include "Core/Rendering/Shader.h"
#include "Core/ResourceManager.h"
#include "Core/scene.h"
#include "Core/sprite.h"
#include "Core/transform.h"

#undef main
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	//TODO: User calls this from console instead
	bool frametime = false;

	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-frametime") == 0) {
			frametime = true;
		}
	}
	
	Renderer * renderer = GetOpenGLRenderer();
	Render_currentRenderer = renderer;
	ResourceManager * resMan = new ResourceManager();
	renderer->Init(resMan, "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	if (!renderer->valid) {
		printf("Invalid renderer.\n");
		return 1;
	}

	std::shared_ptr<Scene> scene = resMan->LoadResourceRefCounted<Scene>("Examples/MM/main.scene");
	if (!scene) {
		printf("Error loading scene.\n");
		return 1;
	}
	scene->time.Start();
	scene->BroadcastEvent("BeginPlay");
	while (true) {
		scene->input->Frame();
		scene->time.Frame();
		//TODO: substeps
		scene->physicsWorld->world->stepSimulation(scene->time.GetDeltaTime());
		scene->BroadcastEvent("Tick", { {"deltaTime", scene->time.GetDeltaTime()} });
		renderer->EndFrame();
		if (frametime) {
			printf("CPU: %f ms GPU: %f ms\n", scene->time.GetDeltaTime() * 1000.f, renderer->GetFrameTime() / 1000000.f);
		}
	}

	renderer->Destroy();
	SDL_Quit();
	return 0;
}
