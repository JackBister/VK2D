#pragma once
#include <Core/Components/Component.h>

class PaddleComponent : public Component
{
public:
	PaddleComponent() { receiveTicks = true; };

	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args = {}) override;

private:
	float flapSpeed = 40.f;
	float gravity = 50.f;
	bool isColliding = false;
	float velocityY = 0.f;
};
