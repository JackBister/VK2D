#pragma once

#include <atomic>

#include "Core/Components/Component.h"

class BufferHandle;
class DescriptorSet;

class SpriteComponent final : public Component
{
public:
	static Deserializable * s_Deserialize(std::string const& str);

	SpriteComponent() { receiveTicks = true; }
	~SpriteComponent() override;

	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args) override;

private:
	DescriptorSet * descriptorSet;
	std::atomic<bool> hasCreatedLocalResources{ false };
	BufferHandle * uniforms;
	
	std::string file;
};
