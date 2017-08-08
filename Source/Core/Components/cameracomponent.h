#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"

class CameraComponent : public Component
{
public:
	CameraComponent() noexcept;

	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	void OnEvent(std::string name, EventArgs args = {}) override;

	glm::mat4 const& projection();
	glm::mat4 const& view();

	PROPERTY(LuaRead)
	float aspect();
	PROPERTY(LuaRead)
	void set_aspect(float);

	PROPERTY(LuaRead)
	float view_size();
	PROPERTY(LuaRead)
	void set_view_size(float);

private:
	float aspect_;
	bool is_projection_dirty_ = true;
	bool is_view_dirty_ = true;
	glm::mat4 projection_;
	glm::mat4 view_;
	float view_size_;
};
