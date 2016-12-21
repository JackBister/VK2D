#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"
#include "Core/Rendering/render.h"

struct Framebuffer;
struct Image;

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
	std::shared_ptr<Framebuffer> GetRenderTarget() const;
	const glm::mat4& GetViewMatrix();

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;

	void OnEvent(std::string name, EventArgs args = {}) override;

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

	std::shared_ptr<Framebuffer> renderTarget;
};
