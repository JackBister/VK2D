#include "Core/transform.h"

#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("Transform");

REFLECT_STRUCT_BEGIN(Transform)
REFLECT_STRUCT_MEMBER(position)
REFLECT_STRUCT_MEMBER(rotation)
REFLECT_STRUCT_MEMBER(scale)
REFLECT_STRUCT_END()

glm::mat4 const & Transform::GetLocalToParent()
{
    glm::mat4 trans = glm::translate(glm::mat4(1.f), position);
    glm::mat4 rotXYZ = glm::mat4_cast(rotation);
    glm::mat4 transrot = trans * rotXYZ;
    glm::mat4 scalem = glm::scale(glm::mat4(1.f), scale);
    toParent = transrot * scalem;
    isParentDirty = false;
    return toParent;
}

glm::mat4 const & Transform::GetLocalToWorld()
{
    if (isWorldDirty) {
        if (parent == nullptr) {
            toWorld = GetLocalToParent();
        } else {
            toWorld = parent->GetLocalToWorld() * GetLocalToParent();
        }
        isWorldDirty = false;
    }
    return toWorld;
}

Transform * Transform::GetParent() const
{
    return parent;
}

glm::vec3 const & Transform::GetPosition() const
{
    return position;
}

glm::quat const & Transform::GetRotation() const
{
    return rotation;
}

glm::vec3 const & Transform::GetScale() const
{
    return scale;
}

SerializedObject Transform::Serialize() const
{
    return SerializedObject::Builder()
        .WithObject("position",
                    SerializedObject::Builder()
                        .WithNumber("x", position.x)
                        .WithNumber("y", position.y)
                        .WithNumber("z", position.z)
                        .Build())
        .WithObject("rotation",
                    SerializedObject::Builder()
                        .WithNumber("x", rotation.x)
                        .WithNumber("y", rotation.y)
                        .WithNumber("z", rotation.z)
                        .WithNumber("w", rotation.w)
                        .Build())
        .WithObject("scale",
                    SerializedObject::Builder()
                        .WithNumber("x", scale.x)
                        .WithNumber("y", scale.y)
                        .WithNumber("z", scale.z)
                        .Build())
        .Build();
}

void Transform::SetParent(Transform * p)
{
    parent = p;
    isParentDirty = true;
    isWorldDirty = true;
}

void Transform::SetPosition(glm::vec3 const & p)
{
    position = p;
    isParentDirty = true;
    isWorldDirty = true;
}

void Transform::SetRotation(glm::quat const & r)
{
    rotation = r;
    isParentDirty = true;
    isWorldDirty = true;
}

void Transform::SetScale(glm::vec3 const & s)
{
    scale = s;
    isParentDirty = true;
    isWorldDirty = true;
}

Transform Transform::Deserialize(SerializedObject const & obj)
{
    Transform ret;
    auto position = obj.GetObject("position").value();
    ret.position.x = position.GetNumber("x").value();
    ret.position.y = position.GetNumber("y").value();
    ret.position.z = position.GetNumber("z").value();

    auto rotation = obj.GetObject("rotation").value();
    ret.rotation.x = rotation.GetNumber("x").value();
    ret.rotation.y = rotation.GetNumber("y").value();
    ret.rotation.z = rotation.GetNumber("z").value();
    ret.rotation.w = rotation.GetNumber("w").value();

    auto scale = obj.GetObject("scale").value();
    ret.scale.x = scale.GetNumber("x").value();
    ret.scale.y = scale.GetNumber("y").value();
    ret.scale.z = scale.GetNumber("z").value();

    return ret;
}
