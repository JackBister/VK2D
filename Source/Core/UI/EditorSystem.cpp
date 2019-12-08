#include "Core/UI/EditorSystem.h"

#include <cstdio>
#include <imgui.h>
#include <optick/optick.h>

#include "Core/GameModule.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/entity.h"
#define REFLECT_IMPL
#include "Core/Reflect.h"
#undef REFLECT_IMPL
#include "Core/dtime.h"

static const auto logger = Logger::Create("EditorSystem");

void DrawEditorNode(EditorNode * e)
{
    switch (e->type) {
    case EditorNode::TREE: {
        auto tree = std::move(std::get<EditorNode::Tree>(e->node));
        if (ImGui::TreeNode(tree.label.c_str())) {
            for (auto const & m : tree.children) {
                DrawEditorNode(m.get());
            }
            ImGui::TreePop();
        }
        break;
    }
    case EditorNode::BOOL: {
        auto b = std::move(std::get<EditorNode::Bool>(e->node));
        ImGui::Text(b.label.c_str());
        ImGui::SameLine();
        ImGui::PushID((int)b.v);
        ImGui::Checkbox("##hidelabel", b.v);
        ImGui::PopID();
        break;
    }
    case EditorNode::FLOAT: {
        auto f = std::move(std::get<EditorNode::Float>(e->node));
        ImGui::Text(f.label.c_str());
        ImGui::SameLine();
        ImGui::PushID((int)f.v);
        ImGui::InputFloat("##hidelabel", f.v, 0.f, 0.f, -1, f.extra_flags);
        ImGui::PopID();
        break;
    }
    case EditorNode::INT: {
        auto i = std::move(std::get<EditorNode::Int>(e->node));
        ImGui::Text(i.label.c_str());
        ImGui::SameLine();
        ImGui::PushID((int)i.v);
        ImGui::InputInt("##hidelabel", i.v, 1, 100, i.extra_flags);
        ImGui::PopID();
        break;
    }
    case EditorNode::NULLPTR: {
        auto n = std::move(std::get<EditorNode::NullPtr>(e->node));
        ImGui::Text(n.label.c_str());
        ImGui::SameLine();
        ImGui::Text("<nullptr>");
        break;
    }
    default: {
        logger->Warnf("Unknown EditorNode type=%zu", e->type);
    }
    }
}

namespace EditorSystem
{
bool isEditorOpen = false;
bool isWorldPaused = false;

const float CAMERA_DRAG_MULTIPLIER_MIN = 5.f;
const float CAMERA_DRAG_MULTIPLIER_MAX = 100.f;
const float CAMERA_DRAG_INCREASE_STEP = 20.f;
float cameraDragMultiplier = 30.f;
Transform cameraTransformBeforeEditorWasOpened;

// Entity Editor state
bool isEntityEditorOpen = false;
struct {
    Entity * currEntity = nullptr;
    size_t currEntityIndex = 0;
} entityEditor;

void OnGui()
{
    OPTICK_EVENT();
    auto io = ImGui::GetIO();
    if (Input::GetKeyDown(KC_F7)) {
        isEditorOpen = !isEditorOpen;
        // Default to pausing when opening editor
        if (isEditorOpen) {
            cameraTransformBeforeEditorWasOpened = GameModule::GetMainCamera()->transform;
            isWorldPaused = true;
            // TODO: Stash timescale and restore it on unpause (both here and in regular pause part)
            Time::SetTimeScale(0.f);
        } else {
            GameModule::GetMainCamera()->transform = cameraTransformBeforeEditorWasOpened;
            isWorldPaused = false;
            Time::SetTimeScale(1.f);
        }
    }
    if (isEditorOpen) {
        if (isWorldPaused) {
            auto cameraPos = GameModule::GetMainCamera()->transform.GetPosition();
            auto cameraRot = GameModule::GetMainCamera()->transform.GetRotation();
            if (Input::GetKey(KC_LALT)) {
                if (io.MouseWheel != 0) {
                    cameraDragMultiplier += io.MouseWheel * CAMERA_DRAG_INCREASE_STEP;
                    if (cameraDragMultiplier < CAMERA_DRAG_MULTIPLIER_MIN) {
                        cameraDragMultiplier = CAMERA_DRAG_MULTIPLIER_MIN;
                    } else if (cameraDragMultiplier > CAMERA_DRAG_MULTIPLIER_MAX) {
                        cameraDragMultiplier = CAMERA_DRAG_MULTIPLIER_MAX;
                    }
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
                if (ImGui::IsMouseDragging()) {
                    auto dragDelta = ImGui::GetMouseDragDelta();
                    ImGui::ResetMouseDragDelta();
                    glm::vec2 delta(-dragDelta.x * cameraDragMultiplier, dragDelta.y * cameraDragMultiplier);
                    delta *= Time::GetUnscaledDeltaTime();
                    cameraPos.x += delta.x;
                    cameraPos.y += delta.y;
                }
            } else if (ImGui::IsMouseDragging(1)) {
                auto dragDelta = ImGui::GetMouseDragDelta(1);
                ImGui::ResetMouseDragDelta(1);

                cameraRot =
                    glm::rotate(glm::mat4(1.f), dragDelta.x * Time::GetUnscaledDeltaTime(), glm::vec3(0.f, -1.f, 0.f)) *
                    glm::mat4_cast(cameraRot);
                cameraRot =
                    glm::mat4_cast(cameraRot) *
                    glm::rotate(glm::mat4(1.f), dragDelta.y * Time::GetUnscaledDeltaTime(), glm::vec3(-1.f, 0.f, 0.f));
            }

            if (Input::GetKey(KC_w)) {
                auto fwd = GameModule::GetMainCamera()->transform.GetLocalToWorld() * glm::vec4(0.f, 0.f, -1.f, 0.f);
                fwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                cameraPos.x += fwd.x;
                cameraPos.y += fwd.y;
                cameraPos.z += fwd.z;
            } else if (Input::GetKey(KC_s)) {
                auto bwd = GameModule::GetMainCamera()->transform.GetLocalToWorld() * glm::vec4(0.f, 0.f, 1.f, 0.f);
                bwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                cameraPos.x += bwd.x;
                cameraPos.y += bwd.y;
                cameraPos.z += bwd.z;
            }

            if (Input::GetKey(KC_a)) {
                auto left = GameModule::GetMainCamera()->transform.GetLocalToWorld() * glm::vec4(-1.f, 0.f, 0.f, 0.f);
                left *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                cameraPos.x += left.x;
                cameraPos.y += left.y;
                cameraPos.z += left.z;
            } else if (Input::GetKey(KC_d)) {
                auto right = GameModule::GetMainCamera()->transform.GetLocalToWorld() * glm::vec4(1.f, 0.f, 0.f, 0.f);
                right *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                cameraPos.x += right.x;
                cameraPos.y += right.y;
                cameraPos.z += right.z;
            }

            GameModule::GetMainCamera()->transform.SetPosition(cameraPos);
            GameModule::GetMainCamera()->transform.SetRotation(cameraRot);
        }
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Entities")) {
                if (ImGui::MenuItem("Entity Editor", nullptr, &isEntityEditorOpen)) {
                    entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
                }
                ImGui::EndMenu();
            }
            if (isWorldPaused && ImGui::MenuItem("Play")) {
                Time::SetTimeScale(1.f);
                isWorldPaused = false;
            } else if (!isWorldPaused && ImGui::MenuItem("Pause")) {
                Time::SetTimeScale(0.f);
                isWorldPaused = true;
            }
            ImGui::EndMainMenuBar();
        }

        if (isEntityEditorOpen && ImGui::Begin("Entity Editor")) {
            if (entityEditor.currEntityIndex != 0 && ImGui::SmallButton("Prev")) {
                entityEditor.currEntityIndex--;
                entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Next")) {
                entityEditor.currEntityIndex++;
                entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
                if (entityEditor.currEntity == nullptr) {
                    entityEditor.currEntityIndex = 0;
                    entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
                }
            }
            if (entityEditor.currEntity != nullptr) {
                ImGui::Text(entityEditor.currEntity->name.c_str());
                ImGui::Separator();
                auto eDesc = reflect::TypeResolver<Entity>::get(entityEditor.currEntity);
                auto e = eDesc->DrawEditorGui("Entity", entityEditor.currEntity, false);
                DrawEditorNode(e.get());
            }
            ImGui::End();
        }
    }
}
}
