#include "Core/scene.h"

#include <fstream>
#include <sstream>

#include "json.hpp"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Framebuffer.h"
#include "Core/sprite.h"
#include "Core/transform.h"

#include "Core/scene.h.generated.h"

Scene::Scene(const std::string& name, ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue,
			 Queue<RenderCommand>::Writer&& writer, Queue<ViewDef *>::Reader&& viewDefQueue, const std::string& serializedScene) noexcept
	: resourceManager(resMan), input(std::move(inputQueue)), renderQueue(std::move(writer)), viewDefQueue(std::move(viewDefQueue))
{
	using nlohmann::json;
	this->name = name;
	json j = json::parse(serializedScene);
	input.DeserializeInPlace(j["input"].dump());
	this->physicsWorld = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(resourceManager, j["physics"].dump()));
	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resourceManager, je.dump()));
		tmp->scene = this;
		this->entities.push_back(tmp);
	}

	//TODO: parametrize
	viewDefs = std::vector<ViewDef>(2);
	for (auto& vd : viewDefs) {
		PushRenderCommand(RenderCommand(RenderCommand::DrawViewParams(&vd)));
	}
}

void Scene::EndFrame() noexcept
{
	Maybe<ViewDef *> nextViewDef = viewDefQueue.Pop();
	//TODO: We can either wait for view def to become available or skip
	//Skipping can starve GPU thread, waiting can stall CPU thread
	//Optimally want to just fire DRAW_VIEWs and let GPU thread work out which one to use.
	/*
	if (nextViewDef.index() == 0) {
		camerasToSubmit.clear();
		spritesToSubmit.clear();
		return;
	}
	*/
	do {
		nextViewDef = viewDefQueue.Pop();
	} while (nextViewDef.index() == 0);

	ViewDef * vd = std::experimental::get<ViewDef *>(nextViewDef);
	std::swap(vd->camera, camerasToSubmit);
	std::swap(vd->sprites, spritesToSubmit);
	PushRenderCommand(RenderCommand(RenderCommand::DrawViewParams(vd)));
	camerasToSubmit.clear();
	spritesToSubmit.clear();
}

void Scene::PushRenderCommand(const RenderCommand& rc) noexcept
{
	renderQueue.Push(rc);
}

void Scene::SubmitCamera(CameraComponent * cam) noexcept
{
	camerasToSubmit.emplace_back(cam->GetViewMatrix(), cam->GetProjectionMatrix(), cam->GetRenderTarget()->GetRendererData());
}

void Scene::SubmitSprite(Sprite * sprite) noexcept
{
	spritesToSubmit.emplace_back(sprite->transform->GetLocalToWorldSpace(), sprite->minUV, sprite->sizeUV, sprite->image->GetRendererData());
}

Entity * Scene::GetEntityByName(std::string name)
{
	for (auto& ep : entities) {
		if (ep->name == name) {
			return ep;
		}
	}
	return nullptr;
}

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities) {
		ep->FireEvent(ename, eas);
	}
}
