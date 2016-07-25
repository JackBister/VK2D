#pragma once

#include "glm/glm.hpp"

#include "component.h"
#include "entity.h"
#include "render.h"

//TODO: Render targets
struct CameraComponent : Component
{	
	CameraComponent()
	{
		receiveTicks = false;
	}

	float GetAspect();
	void SetAspect(float);
	float GetViewSize();
	void SetViewSize(float);

	const glm::mat4& GetProjectionMatrix();

	const glm::mat4& GetViewMatrix();

	Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args = {}) override;
	void PushToLua(lua_State *) override;

	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);

private:
	float aspect;
	bool dirtyProj = true;
	glm::mat4 projMatrix;
	bool dirtyView = true;
	glm::mat4 viewMatrix;
	Renderer * renderer;
	float viewSize;
};

COMPONENT_HEADER(CameraComponent)
