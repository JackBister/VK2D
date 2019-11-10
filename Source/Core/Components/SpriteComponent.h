#pragma once

#include "Core/Components/Component.h"
#include "Core/sprite.h"

class SpriteComponent final : public Component
{
public:
	static Deserializable * s_Deserialize(ResourceManager *, std::string const& str);

	SpriteComponent() { receiveTicks = true; }
	~SpriteComponent() override;

	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args) override;

	Sprite sprite;

private:
	DescriptorSet * descriptorSet;
	std::atomic<bool> hasCreatedLocalResources{ false };
	BufferHandle * uvs;
	
	std::string file;
};
