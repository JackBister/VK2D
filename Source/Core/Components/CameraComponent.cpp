#include "Core/Components/CameraComponent.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Core/GameModule.h"
#include "Core/Logging/Logger.h"
#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"
#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/dtime.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("CameraComponent");

COMPONENT_IMPL(CameraComponent, CameraComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(CameraComponent)
REFLECT_STRUCT_MEMBER(isProjectionDirty)
REFLECT_STRUCT_MEMBER(isViewDirty)
REFLECT_STRUCT_MEMBER(projection)
REFLECT_STRUCT_MEMBER(view)
REFLECT_STRUCT_END()

CameraComponent::~CameraComponent()
{
    auto descriptorSet = this->descriptorSet;
    auto uniforms = this->uniforms;
    ResourceManager::DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyDescriptorSet(descriptorSet);
        ctx.DestroyBuffer(uniforms);
    });
}

glm::mat4 const & CameraComponent::GetProjection()
{
    if (isProjectionDirty) {
        if (cameraData.index() == CameraType::ORTHO) {
            auto cam = std::get<OrthoCamera>(cameraData);
            projection = glm::ortho(-cam.viewSize / cam.aspect, cam.viewSize / cam.aspect, -cam.viewSize, cam.viewSize);
        } else {
            auto cam = std::get<PerspectiveCamera>(cameraData);
            projection = glm::perspective(cam.fov, cam.aspect, cam.zNear, cam.zFar);
        }
        isProjectionDirty = false;
    }
    return projection;
}

glm::mat4 const & CameraComponent::GetView()
{
    if (isViewDirty) {
        view = glm::inverse(entity->transform.GetLocalToWorld());
    }
    return view;
}

Deserializable * CameraComponent::s_Deserialize(DeserializationContext * deserializationContext,
                                                SerializedObject const & obj)
{
    CameraComponent * ret = new CameraComponent();

    auto orthoOpt = obj.GetObject("ortho");
    auto perspectiveOpt = obj.GetObject("perspective");
    if (orthoOpt.has_value()) {
        auto ortho = orthoOpt.value();
        OrthoCamera cam;
        cam.aspect = ortho.GetNumber("aspect").value();
        cam.viewSize = ortho.GetNumber("viewSize").value();
        ret->cameraData = cam;
    } else if (perspectiveOpt.has_value()) {
        auto perspective = perspectiveOpt.value();
        PerspectiveCamera cam;
        cam.aspect = perspective.GetNumber("aspect").value();
        cam.fov = perspective.GetNumber("fov").value();
        cam.zFar = perspective.GetNumber("zFar").value();
        cam.zNear = perspective.GetNumber("zNear").value();
        ret->cameraData = cam;
    } else {
        logger->Errorf("CameraComponent must be initialized with either an 'ortho' parameter or a 'perspective' "
                       "parameter. Will create a default ortho camera.");
        OrthoCamera cam;
        cam.aspect = 0.75;
        cam.viewSize = 60;
        ret->cameraData = cam;
    }

    auto defaultsToMainOpt = obj.GetBool("defaultsToMain");
    if (defaultsToMainOpt.has_value()) {
        ret->defaultsToMain = defaultsToMainOpt.value();
    }

    auto layout =
        ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraPt.layout");
    ResourceManager::CreateResources([ret, layout](ResourceCreationContext & ctx) {
        ret->uniforms = ctx.CreateBuffer({sizeof(glm::mat4),
                                          BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                                          DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
            ret->uniforms, 0, sizeof(glm::mat4)};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};

        ret->descriptorSet = ctx.CreateDescriptorSet({1, descriptors, layout});

        ret->hasCreatedLocalResources = true;
    });

    return ret;
}

SerializedObject CameraComponent::Serialize() const
{
    SerializedObject::Builder builder;
    builder.WithString("type", this->Reflection.name).WithBool("defaultsToMain", defaultsToMain);

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
        logger->Errorf(
            "Unknown cameraData.index() %zu, will not be able to serialize CameraComponent on Entity='%s' fully.",
            cameraData.index(),
            entity->name.c_str());
    }

    return builder.Build();
}

void CameraComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "BeginPlay" && defaultsToMain) {
        GameModule::TakeCameraFocus(entity);
    } else if (name == "TakeCameraFocus") {
        GameModule::TakeCameraFocus(entity);
    } else if (name == "Tick") {
        if (hasCreatedLocalResources) {
            SubmittedCamera submittedCamera;
            submittedCamera.descriptorSet = descriptorSet;
            submittedCamera.projection = GetProjection();
            submittedCamera.view = GetView();
            submittedCamera.uniforms = uniforms;
            GameModule::SubmitCamera(submittedCamera);
        }
    }
}
