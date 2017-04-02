#include "Core/scene.h"

#include <cinttypes>
#include <fstream>
#include <sstream>

#include "json.hpp"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Mesh.h"
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

	for (auto& jr : j["resources"]) {
		std::string fileName = jr["uri"];
		FILE * f = fopen(fileName.c_str(), "rb");
		std::string resourceFile;
		if (f) {
			fseek(f, 0, SEEK_END);
			size_t length = ftell(f);
			fseek(f, 0, SEEK_SET);
			std::vector<char> buf(length + 1);
			fread(&buf[0], 1, length, f);
			resourceFile = std::string(buf.begin(), buf.end());
			fclose(f);
		} else {
			printf("[ERROR] Unable to load resource file %s\n", fileName.c_str());
		}
		json r = json::parse(resourceFile);

		for (auto& accessor : r["Accessors"]) {
			Accessor * a = static_cast<Accessor *>(Deserializable::DeserializeString(resourceManager, accessor.dump()));
			accessors.emplace_back(a);
			resourceManager->AddResourceRefCounted<Accessor>(fileName + "/" + accessor["name"].get<std::string>(), std::weak_ptr<Accessor>(accessors.back()));
		}

		for (auto& program : r["Programs"]) {
			Program * p = static_cast<Program *>(Deserializable::DeserializeString(resourceManager, program.dump()));
			programs.emplace_back(p);
			resourceManager->AddResourceRefCounted<Program>(fileName + "/" + program["name"].get<std::string>(), std::weak_ptr<Program>(programs.back()));
		}

		for (auto& material : r["Materials"]) {
			Material * m = static_cast<Material *>(Deserializable::DeserializeString(resourceManager, material.dump()));
			materials.emplace_back(m);
			resourceManager->AddResourceRefCounted<Material>(fileName + "/" + material["name"].get<std::string>(), std::weak_ptr<Material>(materials.back()));
		}

		for (auto& mesh : r["Meshes"]) {
			Mesh * m = static_cast<Mesh *>(Deserializable::DeserializeString(resourceManager, mesh.dump()));
			meshes.emplace_back(m);
			resourceManager->AddResourceRefCounted<Mesh>(fileName + "/" + mesh["name"].get<std::string>(), std::weak_ptr<Mesh>(meshes.back()));
		}
	}

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

void Scene::SubmitMesh(Mesh * mesh) noexcept
{
	for (auto& p : mesh->primitives) {
		meshesToSubmit.push_back(p.GetMeshSubmission());
	}
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
