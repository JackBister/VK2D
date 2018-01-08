#include "Core/Components/physicscomponent.h"

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "json.hpp"
#include "rttr/registration.h"

#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Core/scene.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<PhysicsComponent>("PhysicsComponent");
}

COMPONENT_IMPL(PhysicsComponent)

BroadphaseNativeTypes DeserializeShapeType(std::string s)
{
	if (s == "BOX_2D_SHAPE_PROXYTYPE") {
		return BOX_2D_SHAPE_PROXYTYPE;
	} else if (s == "CONVEX_2D_SHAPE_PROXYTYPE") {
		return CONVEX_2D_SHAPE_PROXYTYPE;
	} else if (s == "BOX_SHAPE_PROXYTYPE") {
		return BOX_SHAPE_PROXYTYPE;
	} else if (s == "STATIC_PLANE_PROXYTYPE") {
		return STATIC_PLANE_PROXYTYPE;
	} else {
		return INVALID_SHAPE_PROXYTYPE;
	}
}

std::string SerializeShapeType(BroadphaseNativeTypes i)
{
	switch (i) {
	case BOX_2D_SHAPE_PROXYTYPE:
		return "BOX_2D_SHAPE_PROXYTYPE";
	case CONVEX_2D_SHAPE_PROXYTYPE:
		return "CONVEX_2D_SHAPE_PROXYTYPE";
	case BOX_SHAPE_PROXYTYPE:
		return "BOX_SHAPE_PROXYTYPE";
	case STATIC_PLANE_PROXYTYPE:
		return "STATIC_PLANE_PROXYTYPE";
	default:
		return "INVALID_SHAPE_PROXYTYPE";
	}
}

Deserializable * PhysicsComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(PhysicsComponent));
	PhysicsComponent * ret = new (mem) PhysicsComponent();
	auto j = nlohmann::json::parse(str);
	ret->mass = j["mass"];
	ret->shapeType = DeserializeShapeType(j["shapeType"]);
	switch (ret->shapeType) {
	case BOX_2D_SHAPE_PROXYTYPE:
	{
		//1.f might as well be 0.f or any number really.
		btVector3 shape_info(j["shapeInfo"]["x"], j["shapeInfo"]["y"], 1.f);
		ret->shape = std::make_unique<btBox2dShape>(shape_info);
		break;
	}
	case BOX_SHAPE_PROXYTYPE:
	{
		btVector3 shape_info(j["shapeInfo"]["x"], j["shapeInfo"]["y"], j["shapeInfo"]["z"]);
		ret->shape = std::make_unique<btBoxShape>(shape_info);
		break;
	}
	case INVALID_SHAPE_PROXYTYPE:
		//TODO: Error handling. I mean, the engine's going to crash sooner or later anyway but this isn't very clean.
		printf("[ERROR] PhysicsComponent: Invalid shapeType %s.", j["shapeType"].get<std::string>().c_str());
		return nullptr;
	default:
		printf("[ERROR] PhysicsComponent: Unhandled shapeType %s.", j["shapeType"].get<std::string>().c_str());
		return nullptr;
	}
	if (j.find("isKinematic") != j.end()) {
		ret->isKinematic = j["isKinematic"];
	}
	return ret;
}

void PhysicsComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructionInfo(mass, this, shape.get(), localInertia);
		rigidBody = std::make_unique<btRigidBody>(constructionInfo);
		rigidBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));
		rigidBody->setLinearFactor(btVector3(1.f, 1.f, 0.f));
		rigidBody->setUserPointer(this);
		if (isKinematic) {
			rigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		entity->scene->physicsWorld->world->addRigidBody(rigidBody.get());
		//Needed for kinematics to register collision events
		btBroadphaseProxy *bproxy = rigidBody->getBroadphaseHandle();
		if (bproxy) {
			bproxy->m_collisionFilterGroup = short(btBroadphaseProxy::DefaultFilter);
			bproxy->m_collisionFilterMask = short(btBroadphaseProxy::AllFilter);
		}
	}

	if (name == "EndPlay") {
		if (rigidBody != nullptr) {
			entity->scene->physicsWorld->world->removeCollisionObject(rigidBody.get());
		}
	}
}

void PhysicsComponent::getWorldTransform(btTransform& worldTransform) const
{
	Vec3 const& pos = entity->transform.GetPosition();
	worldTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
	Quat const& rot = entity->transform.GetRotation();
	worldTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
	//TODO:
	//const Vec3& scale = entity->transform.scale;
	//rigidBody->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
}

void PhysicsComponent::setWorldTransform(btTransform const& worldTransform)
{
	auto const& pos = worldTransform.getOrigin();
	entity->transform.SetPosition(Vec3(pos.x(), pos.y(), pos.z()));
	auto const& rot = worldTransform.getRotation();
	//I don't know why, but for some reason the components of the rotation are shifted one step when they're returned here, which is why x = w, etc.
	entity->transform.SetRotation(Quat(rot.w(), rot.x(), rot.y(), rot.z()));
}
