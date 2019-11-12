#include "Core/Components/CameraComponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include "nlohmann/json.hpp"

#include "Core/entity.h"
#include "Core/scene.h"
#include "Core/Rendering/SubmittedCamera.h"

COMPONENT_IMPL(CameraComponent, CameraComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(CameraComponent)
REFLECT_STRUCT_MEMBER(aspect)
REFLECT_STRUCT_MEMBER(isProjectionDirty)
REFLECT_STRUCT_MEMBER(isViewDirty)
REFLECT_STRUCT_MEMBER(projection)
REFLECT_STRUCT_MEMBER(view)
REFLECT_STRUCT_MEMBER(viewSize)
REFLECT_STRUCT_END()

float CameraComponent::GetAspect()
{
	return aspect;
}

void CameraComponent::SetAspect(float f)
{
	if (f != aspect) {
		isProjectionDirty = true;
	}
	aspect = f;
}

float CameraComponent::GetViewSize()
{
	return viewSize;
}

void CameraComponent::SetViewSize(float f)
{
	if (f != viewSize) {
		isProjectionDirty = true;
	}
	viewSize = f;
}

glm::mat4 const& CameraComponent::GetProjection()
{
	if (isProjectionDirty) {
		projection = glm::ortho(-viewSize / aspect, viewSize / aspect, -viewSize, viewSize);
	}
	return projection;
}

glm::mat4 const& CameraComponent::GetView()
{
	if (isViewDirty) {
		view = glm::inverse(entity->transform.GetLocalToWorld());
	}
	return view;
}

Deserializable * CameraComponent::s_Deserialize(ResourceManager * resourceManager, std::string const& str)
{
	CameraComponent * ret = new CameraComponent();
	auto const j = nlohmann::json::parse(str);
	ret->aspect = j["aspect"];
	ret->viewSize = j["viewSize"];

	if (j.find("defaultsToMain") != j.end()) {
		ret->defaultsToMain = j["defaultsToMain"];
	}


	auto layout = resourceManager->GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraPt.layout");
	GameModule::CreateResources([ret, layout](ResourceCreationContext& ctx) {
		ret->uniforms = ctx.CreateBuffer({
				sizeof(glm::mat4),
				BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
				DEVICE_LOCAL_BIT
			});

		ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
			ret->uniforms,
			0,
			sizeof(glm::mat4)
		};

		ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
				{
					DescriptorType::UNIFORM_BUFFER,
					0, uniformDescriptor
				}
		};

		ret->descriptorSet = ctx.CreateDescriptorSet({
				1,
				descriptors,
				layout
			});

		ret->hasCreatedLocalResources = true;
	});

	return ret;
}

std::string CameraComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	j["aspect"] = aspect;
	j["viewSize"] = viewSize;
	j["defaultsToMain"] = defaultsToMain;

	return j.dump();
}

void CameraComponent::OnEvent(HashedString name, EventArgs args)
{
	if (name == "BeginPlay" && defaultsToMain) {
		GameModule::TakeCameraFocus(entity);
	} else if (name == "TakeCameraFocus") {
		GameModule::TakeCameraFocus(entity);
	} else if (name == "CameraEditorDrag") {
		deltaLastFrame = *(glm::vec2 *)args["delta"].asPointer;
	} else if (name == "SetPosition") {
		glm::vec3 newPos = *(glm::vec3 *)args["pos"].asPointer;
		entity->transform.SetPosition(newPos);
	} else if (name == "Tick") {
		auto pos = entity->transform.GetPosition();
		auto unscaledDt = Time::GetUnscaledDeltaTime();
		pos.x += deltaLastFrame.x * unscaledDt;
		pos.y += deltaLastFrame.y * unscaledDt;
		entity->transform.SetPosition(pos);
		deltaLastFrame = glm::vec2(0.f, 0.f);

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
