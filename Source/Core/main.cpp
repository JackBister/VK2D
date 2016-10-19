#include <cstdio>

#include "glm/glm.hpp"
#include "SDL/SDL.h"

#include "entity.h"
#include "render.h"
#include "scene.h"
#include "sprite.h"
#include "transform.h"

//TODO:
#include "Core/ResourceManager.h"
#include "Core/Rendering/Shader.h"

#undef main
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	
	Renderer * renderer = GetOpenGLRenderer();
	Render_currentRenderer = renderer;
	ResourceManager * resMan = new ResourceManager();
	renderer->Init(resMan, "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	if (!renderer->valid) {
		printf("Invalid renderer.\n");
		return 1;
	}

	Scene * scene = Scene::FromFile("Examples/MM/main.scene");
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
