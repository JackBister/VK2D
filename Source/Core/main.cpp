#include <cstdio>

#include "glm/glm.hpp"
#include "SDL/SDL.h"

#include "entity.h"
#include "render.h"
#include "scene.h"
#include "sprite.h"
#include "transform.h"

#undef main
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	
	Renderer * renderer = GetOpenGLRenderer();
	Render_currentRenderer = renderer;
	renderer->Init("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	//SDL_Delay(10000);
	if (!renderer->valid) {
		printf("Invalid renderer.\n");
		return 1;
	}

	Scene * scene = Scene::FromFile("Examples/FlappyPong/main.scene");
	scene->time.Start();
	scene->BroadcastEvent("BeginPlay");
	while (true) {
		scene->input->Frame();
		scene->time.Frame();
		//TODO: substeps
		scene->physicsWorld->world->stepSimulation(scene->time.GetDeltaTime());
		scene->BroadcastEvent("Tick", { {"deltaTime", scene->time.GetDeltaTime()} });
		renderer->EndFrame();
	}

	renderer->Destroy();
	SDL_Quit();
	return 0;
}
