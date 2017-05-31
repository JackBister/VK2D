#pragma once
#include <vector>

#include "btBulletDynamicsCommon.h"

#include "Core/Components/component.h"
#include "Tools/HeaderGenerator/headergenerator.h"

struct PhysicsComponent : Component, btMotionState
{
	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
	/*
		Set the transform of the physics object in the "bullet world".
		Called by bullet automatically before stepping the simulation
	*/
	void getWorldTransform(btTransform&) const override;

	void OnEvent(std::string name, EventArgs args = {}) override;
	/*
		Sets the transform of the entity in the rendered world.
	*/
	void setWorldTransform(const btTransform&) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

private:
	PROPERTY(LuaRead)
	bool isKinematic = false;
	PROPERTY(LuaRead)
	float mass;
	btRigidBody * rigidBody;
	btCollisionShape * shape;
	BroadphaseNativeTypes shapeType;
};
