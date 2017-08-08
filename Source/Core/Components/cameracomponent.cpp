#include "Core/Components/cameracomponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "json.hpp"

#include "Core/entity.h"
#include "Core/scene.h"

#include "Core/Components/cameracomponent.h.generated.h"

COMPONENT_IMPL(CameraComponent)

CameraComponent::CameraComponent() noexcept
{
	receive_ticks_ = true;
}

float CameraComponent::aspect()
{
	return aspect_;
}

void CameraComponent::set_aspect(float f)
{
	if (f != aspect_) {
		is_projection_dirty_ = true;
	}
	aspect_ = f;
}

float CameraComponent::view_size()
{
	return view_size_;
}

void CameraComponent::set_view_size(float f)
{
	if (f != view_size_) {
		is_projection_dirty_ = true;
	}
	view_size_ = f;
}

glm::mat4 const& CameraComponent::projection()
{
	if (is_projection_dirty_) {
		projection_ = glm::ortho(-view_size_ / aspect_, view_size_ / aspect_, -view_size_, view_size_);
	}
	return projection_;
}

glm::mat4 const& CameraComponent::view()
{
	if (is_view_dirty_) {
		view_ = glm::inverse(entity_->transform_.local_to_world());
	}
	return view_;
}

Deserializable * CameraComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(CameraComponent));
	CameraComponent * ret = new (mem) CameraComponent();
	auto const j = nlohmann::json::parse(str);
	ret->aspect_ = j["aspect"];
	ret->view_size_ = j["viewSize"];
	
#if 0
	if (j.find("renderTarget") != j.end()) {
		ret->renderTarget = resourceManager->LoadResource<Framebuffer>(j["renderTarget"]);
	}
#endif

	return ret;
}

void CameraComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "Tick") {
		entity_->scene_->SubmitCamera(this);
	}
}
