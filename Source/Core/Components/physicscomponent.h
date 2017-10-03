#pragma once
#include <vector>

#include "btBulletDynamicsCommon.h"
#include "rttr/rttr_enable.h"

#include "Core/Components/component.h"
#include "Tools/HeaderGenerator/headergenerator.h"

class PhysicsComponent : public Component, public btMotionState
{
	RTTR_ENABLE(Component)
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;

	void OnEvent(std::string name, EventArgs args = {}) override;

	/*
		Set the transform of the physics object in the "bullet world".
		Called by bullet automatically before stepping the simulation
	*/
	void getWorldTransform(btTransform&) const override;
	/*
		Sets the transform of the entity in the rendered world.
	*/
	void setWorldTransform(const btTransform&) override;

private:
	PROPERTY(LuaRead)
	bool is_kinematic_ = false;
	PROPERTY(LuaRead)
	float mass_;
	std::unique_ptr<btRigidBody> rigidbody_;
	std::unique_ptr<btCollisionShape> shape_;
	BroadphaseNativeTypes shape_type_;
};
