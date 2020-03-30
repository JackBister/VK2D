#include "Core/Components/PhysicsComponent.h"

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"

static const auto logger = Logger::Create("PhysicsComponent");

COMPONENT_IMPL(PhysicsComponent, &PhysicsComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(PhysicsComponent)
REFLECT_STRUCT_END()

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

PhysicsComponent::~PhysicsComponent()
{
    GameModule::GetPhysicsWorld()->world->removeRigidBody(rigidBody.get());
}

Deserializable * PhysicsComponent::s_Deserialize(DeserializationContext * deserializationContext,
                                                 SerializedObject const & obj)
{
    PhysicsComponent * ret = new PhysicsComponent();
    ret->mass = obj.GetNumber("mass").value();
    ret->shapeType = DeserializeShapeType(obj.GetString("shapeType").value());

    auto shapeInfo = obj.GetObject("shapeInfo").value();
    switch (ret->shapeType) {
    case BOX_2D_SHAPE_PROXYTYPE: {
        btVector3 shape_info(shapeInfo.GetNumber("x").value(), shapeInfo.GetNumber("y").value(), 1.f);
        ret->shape = std::make_unique<btBox2dShape>(shape_info);
        break;
    }
    case BOX_SHAPE_PROXYTYPE: {
        btVector3 shape_info(
            shapeInfo.GetNumber("x").value(), shapeInfo.GetNumber("y").value(), shapeInfo.GetNumber("z").value());
        ret->shape = std::make_unique<btBoxShape>(shape_info);
        break;
    }
    case INVALID_SHAPE_PROXYTYPE:
        // TODO: Error handling. I mean, the engine's going to crash sooner or later anyway but this isn't very clean.
        logger->Errorf("Invalid shapeType %s.", obj.GetString("shapeType").value().c_str());
        return nullptr;
    default:
        logger->Errorf("Unhandled shapeType %s.", obj.GetString("shapeType").value().c_str());
        return nullptr;
    }
    auto isKinematicOpt = obj.GetBool("isKinematic");
    if (isKinematicOpt.has_value()) {
        ret->isKinematic = isKinematicOpt.value();
    }
    return ret;
}

SerializedObject PhysicsComponent::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", this->Reflection.name)
        .WithNumber("mass", mass)
        .WithString("shapeType", SerializeShapeType(shapeType));

    SerializedObject::Builder shapeInfoBuilder;
    switch (shapeType) {
    case BOX_2D_SHAPE_PROXYTYPE: {
        auto shapeInfo = ((btBox2dShape *)shape.get())->getHalfExtentsWithoutMargin();
        shapeInfoBuilder.WithNumber("x", shapeInfo.getX()).WithNumber("y", shapeInfo.getY()).WithNumber("z", 1.f);
        break;
    }
    case BOX_SHAPE_PROXYTYPE: {
        auto shapeInfo = ((btBoxShape *)shape.get())->getHalfExtentsWithoutMargin();
        shapeInfoBuilder.WithNumber("x", shapeInfo.getX())
            .WithNumber("y", shapeInfo.getY())
            .WithNumber("z", shapeInfo.getZ());
        break;
    }
    case INVALID_SHAPE_PROXYTYPE:
        logger->Errorf("Invalid shapeType %s.", SerializeShapeType(shapeType).c_str());
        break;
    default:
        logger->Errorf("Unhandled shapeType %s.", SerializeShapeType(shapeType).c_str());
        break;
    }

    builder.WithObject("shapeInfo", shapeInfoBuilder.Build());
    builder.WithBool("isKinematic", isKinematic);

    return builder.Build();
}

void PhysicsComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif
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
        GameModule::GetPhysicsWorld()->world->addRigidBody(rigidBody.get());
        // Needed for kinematics to register collision events
        btBroadphaseProxy * bproxy = rigidBody->getBroadphaseHandle();
        if (bproxy) {
            bproxy->m_collisionFilterGroup = short(btBroadphaseProxy::DefaultFilter);
            bproxy->m_collisionFilterMask = short(btBroadphaseProxy::AllFilter);
        }
    }

    if (name == "EndPlay") {
        if (rigidBody != nullptr) {
            GameModule::GetPhysicsWorld()->world->removeCollisionObject(rigidBody.get());
        }
    }
}

void PhysicsComponent::getWorldTransform(btTransform & worldTransform) const
{
    glm::vec3 const & pos = entity->transform.GetPosition();
    worldTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    glm::quat const & rot = entity->transform.GetRotation();
    worldTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
    // TODO:
    // const Vec3& scale = entity->transform.scale;
    // rigidBody->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
}

void PhysicsComponent::setWorldTransform(btTransform const & worldTransform)
{
    auto const & pos = worldTransform.getOrigin();
    entity->transform.SetPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
    auto const & rot = worldTransform.getRotation();
    // I don't know why, but for some reason the components of the rotation are shifted one step when they're returned
    // here, which is why x = w, etc.
    entity->transform.SetRotation(glm::quat(rot.w(), rot.x(), rot.y(), rot.z()));
}
