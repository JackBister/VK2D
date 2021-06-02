#include "Core/Components/PhysicsComponent.h"

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("PhysicsComponent");

BroadphaseNativeTypes DeserializeShapeType(std::string s);

REFLECT_STRUCT_BEGIN(PhysicsComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const PHYSICS_COMPONENT_SCHEMA = SerializedObjectSchema(
    "PhysicsComponent",
    {SerializedPropertySchema::Required("mass", SerializedValueType::DOUBLE),
     SerializedPropertySchema::Required("shapeType", SerializedValueType::STRING),
     SerializedPropertySchema::Optional("isKinematic", SerializedValueType::BOOL),
     // TODO: This is actually a variant type but I don't have a good way of expressing that in the schema right now
     SerializedPropertySchema::OptionalObject("shapeInfoBox", "Vec3"),
     SerializedPropertySchema::OptionalObject("shapeInfoBox2d", "Vec2")},
    {SerializedObjectFlag::IS_COMPONENT});

class PhysicsComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return PHYSICS_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        float mass = obj.GetNumber("mass").value();
        BroadphaseNativeTypes shapeType = DeserializeShapeType(obj.GetString("shapeType").value());
        std::unique_ptr<btCollisionShape> shape;
        switch (shapeType) {
        case BOX_2D_SHAPE_PROXYTYPE: {
            auto shapeInfoOpt = obj.GetObject("shapeInfoBox2d");
            if (!shapeInfoOpt.has_value()) {
                logger->Errorf("Failed to deserialize physics component, shapeType was BOX_2D_SHAPE_PROXYTYPE but "
                               "shapeInfoBox2d was not set");
                return nullptr;
            }
            btVector3 shapeInfo(
                shapeInfoOpt.value().GetNumber("x").value(), shapeInfoOpt.value().GetNumber("y").value(), 1.f);
            shape = std::make_unique<btBox2dShape>(shapeInfo);
            break;
        }
        case BOX_SHAPE_PROXYTYPE: {
            auto shapeInfoOpt = obj.GetObject("shapeInfoBox");
            if (!shapeInfoOpt.has_value()) {
                logger->Errorf("Failed to deserialize physics component, shapeType was BOX_2D_SHAPE_PROXYTYPE but "
                               "shapeInfoBox was not set");
                return nullptr;
            }
            btVector3 shapeInfo(shapeInfoOpt.value().GetNumber("x").value(),
                                shapeInfoOpt.value().GetNumber("y").value(),
                                shapeInfoOpt.value().GetNumber("z").value());
            shape = std::make_unique<btBoxShape>(shapeInfo);
            break;
        }
        case INVALID_SHAPE_PROXYTYPE:
            // TODO: Error handling. I mean, the engine's going to crash sooner or later anyway but this isn't very
            // clean.
            logger->Errorf("Invalid shapeType %s.", obj.GetString("shapeType").value().c_str());
            return nullptr;
        default:
            logger->Errorf("Unhandled shapeType %s.", obj.GetString("shapeType").value().c_str());
            return nullptr;
        }
        auto isKinematicOpt = obj.GetBool("isKinematic");
        bool isKinematic = false;
        if (isKinematicOpt.has_value()) {
            isKinematic = isKinematicOpt.value();
        }
        return new PhysicsComponent(isKinematic, mass, std::move(shape), shapeType);
    }
};

COMPONENT_IMPL(PhysicsComponent, new PhysicsComponentDeserializer())

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
    PhysicsWorld::GetInstance()->world->removeRigidBody(rigidBody.get());
}

SerializedObject PhysicsComponent::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", this->Reflection.name)
        .WithNumber("mass", mass)
        .WithString("shapeType", SerializeShapeType(shapeType));

    switch (shapeType) {
    case BOX_2D_SHAPE_PROXYTYPE: {
        auto shapeInfo = ((btBox2dShape *)shape.get())->getHalfExtentsWithoutMargin();
        builder.WithObject(
            "shapeInfoBox2d",
            SerializedObject::Builder().WithNumber("x", shapeInfo.getX()).WithNumber("y", shapeInfo.getY()).Build());
        break;
    }
    case BOX_SHAPE_PROXYTYPE: {
        auto shapeInfo = ((btBoxShape *)shape.get())->getHalfExtentsWithoutMargin();
        builder.WithObject("shapeInfoBox",
                           SerializedObject::Builder()
                               .WithNumber("x", shapeInfo.getX())
                               .WithNumber("y", shapeInfo.getY())
                               .WithNumber("z", shapeInfo.getY())
                               .Build());
        break;
    }
    case INVALID_SHAPE_PROXYTYPE:
        logger->Errorf("Invalid shapeType %s.", SerializeShapeType(shapeType).c_str());
        break;
    default:
        logger->Errorf("Unhandled shapeType %s.", SerializeShapeType(shapeType).c_str());
        break;
    }

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
        logger->Infof("PhysicsComponent::BeginPlay, entity=%s", entity.Get()->GetName().c_str());
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
        PhysicsWorld::GetInstance()->world->addRigidBody(rigidBody.get());
        // Needed for kinematics to register collision events
        btBroadphaseProxy * bproxy = rigidBody->getBroadphaseHandle();
        if (bproxy) {
            bproxy->m_collisionFilterGroup = short(btBroadphaseProxy::DefaultFilter);
            bproxy->m_collisionFilterMask = short(btBroadphaseProxy::AllFilter);
        }
    }

    if (name == "EndPlay") {
        if (rigidBody != nullptr) {
            PhysicsWorld::GetInstance()->world->removeCollisionObject(rigidBody.get());
        }
    }
}

void PhysicsComponent::getWorldTransform(btTransform & worldTransform) const
{
    auto e = entity.Get();
    if (!e) {
        LogMissingEntity();
        return;
    }
    glm::vec3 const & pos = e->GetTransform()->GetPosition();
    worldTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    glm::quat const & rot = e->GetTransform()->GetRotation();
    worldTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
    // TODO:
    // const Vec3& scale = entity->transform.scale;
    // rigidBody->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
}

void PhysicsComponent::setWorldTransform(btTransform const & worldTransform)
{
    auto const & pos = worldTransform.getOrigin();
    auto e = entity.Get();
    if (!e) {
        LogMissingEntity();
        return;
    }
    e->GetTransform()->SetPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
    auto const & rot = worldTransform.getRotation();
    // I don't know why, but for some reason the components of the rotation are shifted one step when they're returned
    // here, which is why x = w, etc.
    e->GetTransform()->SetRotation(glm::quat(rot.w(), rot.x(), rot.y(), rot.z()));
}
