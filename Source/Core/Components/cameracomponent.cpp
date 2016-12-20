#include "cameracomponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "json.hpp"

#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Image.h"

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

std::shared_ptr<Framebuffer> CameraComponent::GetRenderTarget() const
{
	return renderTarget;
}

const glm::mat4& CameraComponent::GetViewMatrix()
{
	if (dirtyView) {
		viewMatrix = glm::inverse(entity->transform.GetLocalToWorldSpace());
	}
	return viewMatrix;
}

Deserializable * CameraComponent::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(CameraComponent));
	CameraComponent * ret = new (mem) CameraComponent();
	json j = json::parse(str);
	ret->aspect = j["aspect"];
	//TODO: Seeing as the renderer might change during runs this might be a bad idea
	ret->renderer = Render_currentRenderer;
	ret->viewSize = j["viewSize"];
	if (j.find("renderTarget") != j.end()) {
		ret->renderTarget = resourceManager->LoadResourceRefCounted<Framebuffer>(j["renderTarget"]);
	}
	return ret;
}

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
