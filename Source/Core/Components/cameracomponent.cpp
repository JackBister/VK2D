#include "cameracomponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "json.hpp"

#include "luaindex.h"

#include "cameracomponent.h.generated.h"

using nlohmann::json;

COMPONENT_IMPL(CameraComponent)

float CameraComponent::GetAspect()
{
	return aspect;
}

void CameraComponent::SetAspect(float f)
{
	if (f != aspect) {
		dirtyProj = true;
	}
	aspect = f;
}

float CameraComponent::GetViewSize()
{
	return viewSize;
}

void CameraComponent::SetViewSize(float f)
{
	if (f != viewSize) {
		dirtyProj = true;
	}
	viewSize = f;
}

const glm::mat4& CameraComponent::GetProjectionMatrix()
{
	if (dirtyProj) {
		projMatrix = glm::ortho(-viewSize / aspect, viewSize / aspect, -viewSize, viewSize);
	}
	return projMatrix;
}

const glm::mat4& CameraComponent::GetViewMatrix()
{
	if (dirtyView) {
		viewMatrix = glm::inverse(entity->transform.GetLocalToWorldSpace());
	}
	return viewMatrix;
}

Deserializable * CameraComponent::Deserialize(const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(CameraComponent));
	CameraComponent * ret = new (mem) CameraComponent();
	json j = json::parse(str);
	ret->aspect = j["aspect"];
	//TODO: Seeing as the renderer might change during runs this might be a bad idea
	ret->renderer = Render_currentRenderer;
	ret->viewSize = j["viewSize"];
	return ret;
}

/*
Component * CameraComponent::Create(std::string s)
{
	CameraComponent * ret = new CameraComponent();
	json j = json::parse(s);
	ret->aspect = j["aspect"];
	//TODO: Seeing as the renderer might change during runs this might be a bad idea
	ret->renderer = Render_currentRenderer;
	ret->viewSize = j["viewSize"];
	return ret;
}
*/

void CameraComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		renderer->AddCamera(this);
	} else if (name == "RendererChanged") {
		renderer = Render_currentRenderer;
	} else if (name == "EndPlay") {
		renderer->DeleteCamera(this);
	}
}
