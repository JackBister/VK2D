#pragma once
#include <vector>

#include "btBulletDynamicsCommon.h"
#include "rttr/rttr_enable.h"

#include "Core/Components/component.h"

class PhysicsComponent : public Component, public btMotionState
{
	RTTR_ENABLE(Component)
public:
	~PhysicsComponent() override;

	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;

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
	bool isKinematic = false;
	float mass;
	std::unique_ptr<btRigidBody> rigidBody;
	std::unique_ptr<btCollisionShape> shape;
	BroadphaseNativeTypes shapeType;
};
