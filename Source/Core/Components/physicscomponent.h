#pragma once

#include "btBulletDynamicsCommon.h"

#include "component.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct PhysicsComponent : Component, btMotionState
{
	bool Component::receiveTicks = false;

	Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args = {}) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	/*
		Set the transform of the physics object in the "bullet world".
		Called by bullet automatically before stepping the simulation TODO:(?)
	*/
	void getWorldTransform(btTransform&) const override;
	/*
		Sets the transform of the entity in the rendered world.
	*/
	void setWorldTransform(const btTransform&) override;

private:
	PROPERTY(LuaRead)
	float mass;
	btRigidBody * rigidBody;
	btCollisionShape * shape;
	BroadphaseNativeTypes shapeType;
};

COMPONENT_HEADER(PhysicsComponent)
