#include "Core/Components/CameraComponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include <optick/optick.h>

#include "Core/EntityManager.h"
#include "Core/GameModule.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Rendering/RenderSystem.h"
#include "Core/entity.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("CameraComponent");

REFLECT_STRUCT_BEGIN(CameraComponent)
REFLECT_STRUCT_MEMBER(isProjectionDirty)
REFLECT_STRUCT_MEMBER(isViewDirty)
REFLECT_STRUCT_MEMBER(projection)
REFLECT_STRUCT_MEMBER(view)
REFLECT_STRUCT_MEMBER(isActive)
REFLECT_STRUCT_END()

static SerializedObjectSchema CAMERA_COMPONENT_SCHEMA = SerializedObjectSchema(
    "CameraComponent",
    {
        SerializedPropertySchema("ortho", SerializedValueType::OBJECT, {}, "OrthoCamera", false, {"perspective"}),
        SerializedPropertySchema("perspective", SerializedValueType::OBJECT, {}, "PerspectiveCamera", false, {"ortho"}),
        SerializedPropertySchema("defaultsToMain", SerializedValueType::BOOL),
        SerializedPropertySchema("isActive", SerializedValueType::BOOL, {}, "", true),
    },
    {SerializedObjectFlag::IS_COMPONENT});

static SerializedObjectSchema
    ORTHO_CAMERA_SCHEMA("OrthoCamera",
                        {
                            SerializedPropertySchema("aspect", SerializedValueType::DOUBLE, {}, "", true),
                            SerializedPropertySchema("viewSize", SerializedValueType::DOUBLE, {}, "", true),
                        });

static SerializedObjectSchema
    PERSPECTIVE_CAMERA_SCHEMA("PerspectiveCamera",
                              {

                                  SerializedPropertySchema("aspect", SerializedValueType::DOUBLE, {}, "", true),
                                  SerializedPropertySchema("fov", SerializedValueType::DOUBLE, {}, "", true),
                                  SerializedPropertySchema("zFar", SerializedValueType::DOUBLE, {}, "", true),
                                  SerializedPropertySchema("zNear", SerializedValueType::DOUBLE, {}, "", true),
                              });

class CameraComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return CAMERA_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        std::variant<OrthoCamera, PerspectiveCamera> cameraData;
        auto orthoOpt = obj.GetObject("ortho");
        auto perspectiveOpt = obj.GetObject("perspective");
        if (orthoOpt.has_value()) {
            auto ortho = orthoOpt.value();
            OrthoCamera cam;
            cam.aspect = ortho.GetNumber("aspect").value();
            cam.viewSize = ortho.GetNumber("viewSize").value();
            cameraData = cam;
        } else if (perspectiveOpt.has_value()) {
            auto perspective = perspectiveOpt.value();
            PerspectiveCamera cam;
            cam.aspect = perspective.GetNumber("aspect").value();
            cam.fov = perspective.GetNumber("fov").value();
            cam.zFar = perspective.GetNumber("zFar").value();
            cam.zNear = perspective.GetNumber("zNear").value();
            cameraData = cam;
        } else {
            logger.Error("CameraComponent must be initialized with either an 'ortho' parameter or a 'perspective' "
                         "parameter. Will create a default ortho camera.");
            OrthoCamera cam;
            cam.aspect = 0.75;
            cam.viewSize = 60;
        }

        auto defaultsToMain = obj.GetBool("defaultsToMain").value_or(false);
        auto isActive = obj.GetBool("isActive").value();

        return new CameraComponent(cameraData, isActive, defaultsToMain);
    }
};

class OrthoCameraDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return ORTHO_CAMERA_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto ret = new OrthoCamera();
        ret->aspect = obj.GetNumber("aspect").value();
        ret->viewSize = obj.GetNumber("viewSize").value();
        return ret;
    }
};

class PerspectiveCameraDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return PERSPECTIVE_CAMERA_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto ret = new PerspectiveCamera();
        ret->aspect = obj.GetNumber("aspect").value();
        ret->fov = obj.GetNumber("fov").value();
        ret->zFar = obj.GetNumber("zFar").value();
        ret->zNear = obj.GetNumber("zNear").value();
        return ret;
    }
};

COMPONENT_IMPL(CameraComponent, new CameraComponentDeserializer())

DESERIALIZABLE_IMPL(OrthoCamera, new OrthoCameraDeserializer())
DESERIALIZABLE_IMPL(PerspectiveCamera, new PerspectiveCameraDeserializer())

CameraComponent::CameraComponent(std::variant<OrthoCamera, PerspectiveCamera> cameraData, bool isActive,
                                 bool defaultsToMain)
    : cameraData(cameraData), isActive(isActive), defaultsToMain(defaultsToMain)
{
    cameraHandle = RenderSystem::GetInstance()->CreateCamera();

    receiveTicks = false;
    type = "CameraComponent";
}

CameraComponent::~CameraComponent()
{
    RenderSystem::GetInstance()->DestroyCamera(this->cameraHandle);
}

glm::mat4 const & CameraComponent::GetProjection()
{
    if (isProjectionDirty) {
        if (cameraData.index() == CameraType::ORTHO) {
            auto cam = std::get<OrthoCamera>(cameraData);
            projection = glm::ortho(-cam.viewSize / cam.aspect, cam.viewSize / cam.aspect, -cam.viewSize, cam.viewSize);
        } else {
            auto cam = std::get<PerspectiveCamera>(cameraData);
            projection = glm::perspective(glm::radians(cam.fov), cam.aspect, cam.zNear, cam.zFar);
        }
        isProjectionDirty = false;
    }
    return projection;
}

glm::mat4 const & CameraComponent::GetView()
{
    if (isViewDirty) {
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return view;
        }
        view = glm::inverse(e->GetTransform()->GetLocalToWorld());
    }
    return view;
}

SerializedObject CameraComponent::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", this->Reflection.name)
        .WithBool("defaultsToMain", defaultsToMain)
        .WithBool("isActive", isActive);

    if (cameraData.index() == CameraType::ORTHO) {
        auto ortho = std::get<OrthoCamera>(cameraData);
        builder.WithObject("ortho",
                           SerializedObject::Builder()
                               .WithNumber("aspect", ortho.aspect)
                               .WithNumber("viewSize", ortho.viewSize)
                               .Build());
    } else if (cameraData.index() == CameraType::PERSPECTIVE) {
        auto perspective = std::get<PerspectiveCamera>(cameraData);
        builder.WithObject("perspective",
                           SerializedObject::Builder()
                               .WithNumber("aspect", perspective.aspect)
                               .WithNumber("fov", perspective.fov)
                               .WithNumber("zFar", perspective.zFar)
                               .WithNumber("zNear", perspective.zNear)
                               .Build());
    } else {
        auto e = entity.Get();
        logger.Error(
            "Unknown cameraData.index() {}, will not be able to serialize CameraComponent on Entity='{}' fully.",
            cameraData.index(),
            e->GetName());
    }

    return builder.Build();
}

void CameraComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "BeginPlay" && defaultsToMain) {
        auto entityManager = entity.GetManager();
        if (!entityManager) {
            logger.Error("entityManager was null when trying to set main camera in BeginPlay");
            return;
        }
        entityManager->SetSingletonTag(EntityManager::IS_MAIN_CAMERA_TAG, entity);
    } else if (name == "TakeCameraFocus") {
        auto entityManager = entity.GetManager();
        if (!entityManager) {
            logger.Error("entityManager was null when trying to set main camera in TakeCameraFocus");
            return;
        }
        entityManager->SetSingletonTag(EntityManager::IS_MAIN_CAMERA_TAG, entity);
    } else if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        auto e = entity.Get();
        builder->WithCameraUpdate(
            {cameraHandle, e->GetTransform()->GetPosition(), GetView(), GetProjection(), isActive});
    }
}
