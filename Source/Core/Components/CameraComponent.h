#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"

class CameraComponent : public Component
{
public:
	CameraComponent();

	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;

	void OnEvent(std::string name, EventArgs args = {}) override;

	glm::mat4 const& GetProjection();
	glm::mat4 const& GetView();

	float GetAspect();
	void SetAspect(float);

	float GetViewSize();
	void SetViewSize(float);

private:
	float aspect;
	bool isProjectionDirty = true;
	bool isViewDirty = true;
	glm::mat4 projection;
	glm::mat4 view;
	float viewSize;
};
