#pragma once

#include "glm/glm.hpp"

#include "component.h"
#include "entity.h"
#include "render.h"

//TODO: Render targets
struct CameraComponent : Component
{
	bool Component::receiveTicks = false;

	PROPERTY(LuaRead)
	float GetAspect();
	PROPERTY(LuaRead)
	void SetAspect(float);
	PROPERTY(LuaRead)
	float GetViewSize();
	PROPERTY(LuaRead)
	void SetViewSize(float);

	const glm::mat4& GetProjectionMatrix();

	const glm::mat4& GetViewMatrix();

	Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args = {}) override;
	//void PushToLua(lua_State *) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

private:
	float aspect;
	bool dirtyProj = true;
	glm::mat4 projMatrix;
	bool dirtyView = true;
	glm::mat4 viewMatrix;
	Renderer * renderer;
	float viewSize;
};
