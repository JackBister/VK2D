#pragma once
#include <Core/Components/Component.h>

class BallComponent : public Component
{
public:
	static Deserializable * s_Deserialize(ResourceManager *, std::string const& str);

	BallComponent() { receiveTicks = true; };

	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args = {}) override;

private:
	glm::vec2 velocityDir;
	float moveSpeed;
};
