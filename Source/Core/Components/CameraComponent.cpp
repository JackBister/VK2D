#include "Core/Components/cameracomponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "nlohmann/json.hpp"

#include "Core/entity.h"
#include "Core/scene.h"

COMPONENT_IMPL(CameraComponent)

CameraComponent::CameraComponent()
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

Deserializable * CameraComponent::Deserialize(ResourceManager * resourceManager, std::string const& str) const
{
	CameraComponent * ret = new CameraComponent();
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
		GameModule::SubmitCamera(this);
	}
}
