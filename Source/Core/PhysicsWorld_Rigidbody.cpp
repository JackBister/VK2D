#include "physicsworld.h"

#include <ThirdParty/bullet3/src/BulletCollision/CollisionShapes/btBox2dShape.h>
#include <ThirdParty/bullet3/src/btBulletDynamicsCommon.h>

#include "Core/Components/PhysicsComponent.h"
#include "EntityPtr.h"
#include "Logging/Logger.h"
#include "PhysicsWorld_Private.h"
#include "Serialization/Deserializable.h"
#include "Serialization/SchemaValidator.h"
#include "Serialization/SerializedObjectSchema.h"
#include "entity.h"

static auto const logger = Logger::Create("PhysicsWorld");

// TODO: Move this somewhere else
static SerializedObjectSchema const PLANE_SHAPE_INFO_SCHEMA = SerializedObjectSchema(
    "PlaneShapeInfo", {SerializedPropertySchema::RequiredObject("normal", "Vec3"),
                       SerializedPropertySchema::Required("planeConstant", SerializedValueType::DOUBLE)});

class PlaneShapeInfoDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return PLANE_SHAPE_INFO_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        auto normalOpt = obj.GetObject("normal");
        btVector3 normal(normalOpt.value().GetNumber("x").value(),
                         normalOpt.value().GetNumber("y").value(),
                         normalOpt.value().GetNumber("z").value());
        btScalar planeConstant = obj.GetNumber("planeConstant").value();
        return new btStaticPlaneShape(normal, planeConstant);
    }
};

DESERIALIZABLE_IMPL(PlaneShapeInfo, new PlaneShapeInfoDeserializer());

static SerializedObjectSchema const RIGIDBODY_SCHEMA = SerializedObjectSchema(
    "Rigidbody",
    {SerializedPropertySchema::Required("mass", SerializedValueType::DOUBLE),
     SerializedPropertySchema::Required(
         "shapeType", SerializedValueType::STRING,
         SerializedPropertyFlags(
             {StringEnumFlag({"BOX_2D_SHAPE_PROXYTYPE", "BOX_SHAPE_PROXYTYPE", "STATIC_PLANE_PROXYTYPE"})})),
     SerializedPropertySchema::Optional("isKinematic", SerializedValueType::BOOL),
     // TODO: This is actually a variant type but I don't have a good way of expressing that in the schema right now
     SerializedPropertySchema::OptionalObject("shapeInfoBox", "Vec3"),
     SerializedPropertySchema::OptionalObject("shapeInfoBox2d", "Vec2"),
     SerializedPropertySchema::OptionalObject("shapeInfoStaticPlane", "PlaneShapeInfo")});

class RigidbodyDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return RIGIDBODY_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        logger.Error("STUB: RigidbodyDeserializer::Deserialize");
        return nullptr;
    }
};

DESERIALIZABLE_IMPL(Rigidbody, new RigidbodyDeserializer())

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

void MotionStateAdapter::getWorldTransform(btTransform & worldTransform) const
{
    auto e = entity.Get();
    if (!e) {
        logger.Error("RigidbodyInstance with id={}, referred entity does not exist. entity={}", id, entity);
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

void MotionStateAdapter::setWorldTransform(btTransform const & worldTransform)
{
    auto const & pos = worldTransform.getOrigin();
    auto e = entity.Get();
    if (!e) {
        logger.Error("RigidbodyInstance with id={}, referred entity does not exist. entity={}", id, entity);
        return;
    }
    e->GetTransform()->SetPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
    auto const & rot = worldTransform.getRotation();
    e->GetTransform()->SetRotation(glm::quat(rot.w(), rot.x(), rot.y(), rot.z()));
}

std::optional<RigidbodyInstanceId> PhysicsWorld::AddRigidbody(PhysicsComponent * component,
                                                              DeserializationContext * ctx, SerializedObject obj)
{
    auto validationResult = SchemaValidator::Validate(RIGIDBODY_SCHEMA, obj);
    if (!validationResult.isValid) {
        logger.Warn("Failed to deserialize Rigidbody from SerializedObject, validation failed. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger.Warn("\t{}: {}", err.first, err.second);
        }
        return std::nullopt;
    }

    float mass = obj.GetNumber("mass").value();
    BroadphaseNativeTypes shapeType = DeserializeShapeType(obj.GetString("shapeType").value());
    std::unique_ptr<btCollisionShape> shape;
    switch (shapeType) {
    case BOX_2D_SHAPE_PROXYTYPE: {
        auto shapeInfoOpt = obj.GetObject("shapeInfoBox2d");
        if (!shapeInfoOpt.has_value()) {
            logger.Error("Failed to deserialize rigidbody, shapeType was BOX_2D_SHAPE_PROXYTYPE but "
                         "shapeInfoBox2d was not set");
            return std::nullopt;
        }
        btVector3 shapeInfo(
            shapeInfoOpt.value().GetNumber("x").value(), shapeInfoOpt.value().GetNumber("y").value(), 1.f);
        shape = std::make_unique<btBox2dShape>(shapeInfo);
        break;
    }
    case BOX_SHAPE_PROXYTYPE: {
        auto shapeInfoOpt = obj.GetObject("shapeInfoBox");
        if (!shapeInfoOpt.has_value()) {
            logger.Error("Failed to deserialize physics component, shapeType was BOX_2D_SHAPE_PROXYTYPE but "
                         "shapeInfoBox was not set");
            return std::nullopt;
        }
        btVector3 shapeInfo(shapeInfoOpt.value().GetNumber("x").value(),
                            shapeInfoOpt.value().GetNumber("y").value(),
                            shapeInfoOpt.value().GetNumber("z").value());
        shape = std::make_unique<btBoxShape>(shapeInfo);
        break;
    }
    case STATIC_PLANE_PROXYTYPE: {
        auto shapeInfoOpt = obj.GetObject("shapeInfoStaticPlane");
        if (!shapeInfoOpt.has_value()) {
            logger.Error("Failed to deserialize physics component, shapeType was STATIC_PLANE_PROXYTYPE but "
                         "shapeInfoStaticPlane was not set");
            return std::nullopt;
        }
        auto normalOpt = shapeInfoOpt.value().GetObject("normal");
        if (!normalOpt.has_value()) {
            logger.Error("Failed to deserialize physics component, shapeType was STATIC_PLANE_PROXYTYPE but "
                         "shapeInfoStaticPlane.normal was not set");
            return std::nullopt;
        }
        btVector3 normal(normalOpt.value().GetNumber("x").value(),
                         normalOpt.value().GetNumber("y").value(),
                         normalOpt.value().GetNumber("z").value());
        btScalar planeConstant = shapeInfoOpt.value().GetNumber("planeConstant").value();
        shape = std::make_unique<btStaticPlaneShape>(normal, planeConstant);
        break;
    }
    case INVALID_SHAPE_PROXYTYPE:
        // TODO: Error handling. I mean, the engine's going to crash sooner or later anyway but this isn't very
        // clean.
        logger.Error("Invalid shapeType {}.", obj.GetString("shapeType").value());
        return std::nullopt;
    default:
        logger.Error("Unhandled shapeType {}.", obj.GetString("shapeType").value());
        return std::nullopt;
    }
    auto isKinematicOpt = obj.GetBool("isKinematic");
    bool isKinematic = isKinematicOpt.value_or(false);

    RigidbodyInstanceId id = pimpl->currentRigidbodyId.fetch_add(1);
    auto motionState = std::make_unique<MotionStateAdapter>(id, component->entity);

    btVector3 localInertia;
    shape->calculateLocalInertia(mass, localInertia);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(mass, motionState.get(), shape.get(), localInertia);
    auto rigidBody = std::make_unique<btRigidBody>(constructionInfo);
    rigidBody->setAngularFactor(btVector3(0.f, 0.f, 1.f));
    rigidBody->setLinearFactor(btVector3(1.f, 1.f, 0.f));
    rigidBody->setUserPointer(component);
    if (isKinematic) {
        rigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    }
    pimpl->world->addRigidBody(rigidBody.get());
    // Needed for kinematics to register collision events
    btBroadphaseProxy * bproxy = rigidBody->getBroadphaseHandle();
    if (bproxy) {
        bproxy->m_collisionFilterGroup = short(btBroadphaseProxy::DefaultFilter);
        bproxy->m_collisionFilterMask = short(btBroadphaseProxy::AllFilter);
    }

    pimpl->rigidbodyInstances[id] = RigidbodyInstance(
        id, std::move(motionState), std::move(rigidBody), std::move(shape), shapeType, isKinematic, mass);

    return id;
}

void PhysicsWorld::RemoveRigidbody(RigidbodyInstanceId id)
{
    if (pimpl->rigidbodyInstances.find(id) == pimpl->rigidbodyInstances.end()) {
        logger.Error("Tried to remove rigidbody with id={} but it was not found");
    }
    RigidbodyInstance * instance = &pimpl->rigidbodyInstances.at(id);
    pimpl->world->removeRigidBody(instance->rigidBody.get());
    pimpl->rigidbodyInstances.erase(id);
}
