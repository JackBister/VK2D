#pragma once

#include "glm/glm.hpp"

#include "Core/Components/component.h"

struct Framebuffer;
struct Image;

struct CameraComponent : Component
{
public:
	CameraComponent() noexcept;


	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;

	PROPERTY(LuaRead)
	float GetAspect();

	const glm::mat4& GetProjectionMatrix();
	std::shared_ptr<Framebuffer> GetRenderTarget() const;
	const glm::mat4& GetViewMatrix();

	PROPERTY(LuaRead)
	float GetViewSize();

	void OnEvent(std::string name, EventArgs args = {}) override;

	PROPERTY(LuaRead)
	void SetAspect(float);

	PROPERTY(LuaRead)
	void SetViewSize(float);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
private:
	float aspect;
	bool dirtyProj = true;
	bool dirtyView = true;
	glm::mat4 projMatrix;
	std::shared_ptr<Framebuffer> renderTarget;
	glm::mat4 viewMatrix;
	float viewSize;

};
