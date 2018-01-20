#pragma once
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"

#include "Core/Components/component.h"
#include "Core/sprite.h"

class SpritesheetComponent : public Component
{
public:
	static Deserializable * s_Deserialize(ResourceManager *, std::string const& str);

	SpritesheetComponent() { receiveTicks = false; }

	std::string Serialize() const override;
	glm::vec2 get_frame_size() const;
	void OnEvent(HashedString, EventArgs) override;
	void PlayAnimationByName(std::string name);

	Sprite sprite_;

	bool is_flipped_;

	std::vector<float> frameTimes;
private:
	struct SpriteResources
	{
		BufferHandle * ebo;
		BufferHandle * vbo;

		PipelineHandle * pipeline;
	};

	static bool s_hasCreatedResources;
	static SpriteResources s_resources;

	glm::mat4 cachedMvp;
	DescriptorSet * descriptorSet;
	std::atomic<bool> hasCreatedLocalResources{ false };
	BufferHandle * uvs;

	std::unordered_map<std::string, std::vector<size_t>> animations;
	size_t currentIndex = 0;
	std::string currentNamedAnim;
	size_t currentNamedAnimIndex;
	glm::vec2 frameSize;
	std::vector<glm::vec2> minUvs;
	float timeSinceUpdate = 0.f;
};
