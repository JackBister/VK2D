#include "Core/Components/cameracomponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "nlohmann/json.hpp"
#include "rttr/registration.h"

#include "Core/entity.h"
#include "Core/scene.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<CameraComponent>("CameraComponent")
	.constructor<>()
	.method("projection", &CameraComponent::GetProjection)
	.method("view", &CameraComponent::GetView)
	.method("aspect", &CameraComponent::GetAspect)
	.method("set_aspect", &CameraComponent::SetAspect)
	.method("view_size", &CameraComponent::GetViewSize)
	.method("set_view_size", &CameraComponent::SetViewSize);
}

COMPONENT_IMPL(CameraComponent)

CameraComponent::CameraComponent() noexcept
{
	receiveTicks = true;
}

float CameraComponent::GetAspect()
{
	return aspect;
}

void CameraComponent::SetAspect(float f)
{
	if (f != aspect) {
		isProjectionDirty = true;
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
		isProjectionDirty = true;
	}
	viewSize = f;
}

glm::mat4 const& CameraComponent::GetProjection()
{
	if (isProjectionDirty) {
		projection = glm::ortho(-viewSize / aspect, viewSize / aspect, -viewSize, viewSize);
	}
	return projection;
}

glm::mat4 const& CameraComponent::GetView()
{
	if (isViewDirty) {
		view = glm::inverse(entity->transform.GetLocalToWorld());
	}
	return view;
}

Deserializable * CameraComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(CameraComponent));
	CameraComponent * ret = new (mem) CameraComponent();
	auto const j = nlohmann::json::parse(str);
	ret->aspect = j["aspect"];
	ret->viewSize = j["viewSize"];

	return ret;
}

std::string CameraComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	j["aspect"] = aspect;
	j["viewSize"] = viewSize;

	return j.dump();
}

void CameraComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "Tick") {
		entity->scene->SubmitCamera(this);
	}
}
