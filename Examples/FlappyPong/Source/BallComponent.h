#pragma once
#include <Core/Components/Component.h>

class BallComponent : public Component
{
public:
	BallComponent() { receiveTicks = true; };

	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args = {}) override;

private:
	glm::vec2 velocityDir;
	float moveSpeed;
};
