#include "physicscomponent.h"

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "json.hpp"

#include "entity.h"
#include "luaindex.h"
#include "scene.h"

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

Component * PhysicsComponent::Create(std::string json)
{
	PhysicsComponent * ret = new PhysicsComponent();
	auto j = nlohmann::json::parse(json);
	ret->mass = j["mass"];
	ret->shapeType = DeserializeShapeType(j["shapeType"]);
	switch (ret->shapeType) {
	case BOX_2D_SHAPE_PROXYTYPE:
	{
		//1.f might as well be 0.f or any number really.
		btVector3 shapeInfo(j["shapeInfo"]["x"], j["shapeInfo"]["y"], 1.f);
		ret->shape = new btBox2dShape(shapeInfo);
		break;
	}
	case BOX_SHAPE_PROXYTYPE:
	{
		btVector3 shapeInfo(j["shapeInfo"]["x"], j["shapeInfo"]["y"], j["shapeInfo"]["z"]);
		ret->shape = new btBoxShape(shapeInfo);
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
	return ret;
}

void PhysicsComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		btVector3 localInertia;
		shape->calculateLocalInertia(mass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructionInfo(mass, this, shape, localInertia);
		rigidBody = new btRigidBody(constructionInfo);
		rigidBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));
		rigidBody->setLinearFactor(btVector3(1.f, 1.f, 0.f));
	//	rigidBody->setActivationState(DISABLE_DEACTIVATION);
		entity->scene->physicsWorld.world->addRigidBody(rigidBody);
	}

	if (name == "EndPlay") {
		if (rigidBody != nullptr) {
			entity->scene->physicsWorld.world->removeCollisionObject(rigidBody);
		}
	}
}

LUA_INDEX(PhysicsComponent, float, mass)

int PhysicsComponent::LuaNewIndex(lua_State *)
{
	return 0;
}

void PhysicsComponent::getWorldTransform(btTransform& worldTransform) const
{
	const Vec3& pos = entity->transform.position;
	worldTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
	const Quat& rot = entity->transform.rotation;
	worldTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
//	const Vec3& scale = entity->transform.scale;
	//TODO:
	//rigidBody->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
}

void PhysicsComponent::setWorldTransform(const btTransform& worldTransform)
{
	const btVector3& pos = worldTransform.getOrigin();
	entity->transform.position = Vec3(pos.x(), pos.y(), pos.z());
	const btQuaternion& rot = worldTransform.getRotation();
	//I don't know why, but for some reason the components of the rotation are shifted one step when they're returned here, which is why x = w, etc.
	entity->transform.rotation = Quat(rot.w(), rot.x(), rot.y(), rot.z());
	entity->transform.MakeDirty();
}
