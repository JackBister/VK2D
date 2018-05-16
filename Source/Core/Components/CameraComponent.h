#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"
#include "Core/Math/mathtypes.h"

class CameraComponent : public Component
{
public:
	static Deserializable * s_Deserialize(ResourceManager *, std::string const& str);

	CameraComponent() { receiveTicks = true; };

	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args = {}) override;

	Mat4 const& GetProjection();
	Mat4 const& GetView();

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
	Mat4 projection;
	Mat4 view;
	float viewSize;

	Vec2 deltaLastFrame;
};
