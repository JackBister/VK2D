#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/Deserializable.h"
#include "Core/Reflect.h"
#include "Core/Serialization/SerializedValue.h"
#include "Util/DllExport.h"

class TransformDeserializer;

class EAPI Transform final : public Deserializable
{
public:
    friend class TransformDeserializer;

    static Transform Deserialize(SerializedObject const &);

    glm::mat4 const & GetLocalToParent();
    glm::mat4 const & GetLocalToWorld();
    Transform * GetParent() const;
    glm::vec3 const & GetPosition() const;
    glm::quat const & GetRotation() const;
    glm::vec3 const & GetScale() const;

    SerializedObject Serialize() const override;

    void SetParent(Transform *);
    void SetPosition(glm::vec3 const &);
    void SetRotation(glm::quat const &);
    void SetScale(glm::vec3 const &);

    REFLECT()
private:
    Transform * parent = nullptr;

    glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);

    bool isParentDirty = true;
    bool isWorldDirty = true;

    glm::mat4 toParent;
    glm::mat4 toWorld;
};
