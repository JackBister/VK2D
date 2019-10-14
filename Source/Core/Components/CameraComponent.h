#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"

class CameraComponent : public Component
{
public:
	static Deserializable * s_Deserialize(ResourceManager *, std::string const& str);

	CameraComponent() { receiveTicks = true; };

	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args = {}) override;

	glm::mat4 const& GetProjection();
    glm::mat4 const& GetView();

	float GetAspect();
	void SetAspect(float);

	float GetViewSize();
	void SetViewSize(float);

	REFLECT()
	REFLECT_INHERITANCE()
private:
	float aspect;
	bool defaultsToMain = false;
	bool isProjectionDirty = true;
	bool isViewDirty = true;
    glm::mat4 projection;
    glm::mat4 view;
	float viewSize;

	glm::vec2 deltaLastFrame = glm::vec2(0.0, 0.0);
};
