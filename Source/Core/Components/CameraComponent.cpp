#include "CameraComponent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <optick/optick.h>

#include "Core/EntityManager.h"
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

        return new CameraComponent(RenderSystem::GetInstance(), cameraData, isActive, defaultsToMain);
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

CameraComponent::CameraComponent(RenderSystem * renderSystem, std::variant<OrthoCamera, PerspectiveCamera> cameraData,
                                 bool isActive, bool defaultsToMain)
    : renderSystem(renderSystem), cameraData(cameraData), isActive(isActive), defaultsToMain(defaultsToMain)
{
    cameraHandle = renderSystem->CreateCamera();

    receiveTicks = false;
    type = "CameraComponent";
}

CameraComponent::~CameraComponent()
{
    renderSystem->DestroyCamera(this->cameraHandle);
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

std::optional<Line> CameraComponent::ScreenPointToLine(int x, int y)
{
    if (x < 0 || y < 0) {
        logger.Warn("ScreenPointToLine: x/y cannot be less than 0 but were x={} y={}", x, y);
        return std::nullopt;
    }
    glm::ivec2 resolution = renderSystem->GetResolution();
    if (x > resolution.x || y > resolution.y) {
        logger.Warn("ScreenPointToLine: x/y cannot be greater than resolution but were x={} y={}. resolution was "
                    "resX={} resY={}",
                    x,
                    y,
                    resolution.x,
                    resolution.y);
        return std::nullopt;
    }
    auto e = entity.Get();
    if (!e) {
        LogMissingEntity();
        return std::nullopt;
    }
    float clipX = ((2.f * x) / (float)resolution.x) - 1.f;
    float clipY = 1.f - ((2.f * y) / (float)resolution.y);
    float clipZ = 1.f;
    float zFar = 0.f;
    if (cameraData.index() == 0) {
        auto & orthoCamera = std::get<OrthoCamera>(cameraData);
        zFar = 10000.f;
    } else if (cameraData.index() == 1) {
        auto & perspectiveCamera = std::get<PerspectiveCamera>(cameraData);
        zFar = perspectiveCamera.zFar;
    } else {
        logger.Warn("Unknown camera type, index={}", cameraData.index());
        return std::nullopt;
    }
    glm::vec4 clipCoordinate(clipX, clipY, clipZ, 1.f);
    glm::mat4 v = GetView();
    glm::mat4 p = GetProjection();
    glm::vec4 worldCoordinate = glm::inverse(v) * (glm::inverse(p) * clipCoordinate);
    worldCoordinate = glm::normalize(worldCoordinate);
    worldCoordinate *= zFar;
    auto transform = e->GetTransform();
    auto model = transform->GetLocalToWorld();
    glm::vec4 worldPos = model * glm::vec4(0.f, 0.f, 0.f, 1.f);
    return Line(worldPos, worldCoordinate);
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
