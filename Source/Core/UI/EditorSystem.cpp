#include "Core/UI/EditorSystem.h"

#include <cstdio>
#include <imgui.h>

#include "Core/entity.h"
#include "Core/GameModule.h"
#define REFLECT_IMPL
#include "Core/Reflect.h"
#undef REFLECT_IMPL

void DrawEditorNode(EditorNode * e)
{
	switch (e->type) {
	case EditorNode::TREE: {
		auto tree = std::move(std::get<EditorNode::Tree>(e->node));
		if (ImGui::TreeNode(tree.label.c_str())) {
			for (auto const& m : tree.children) {
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
		printf("[WARNING] Unknown EditorNode type %zu\n", e->type);
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
	Vec3 cameraPosBeforeEditorWasOpened;

	//Entity Editor state
	bool isEntityEditorOpen = false;
	struct {
		Entity * currEntity = nullptr;
		size_t currEntityIndex = 0;
	} entityEditor;

	void OnGui()
	{
		auto io = ImGui::GetIO();
		if (Input::GetKeyDown(KC_F7)) {
			isEditorOpen = !isEditorOpen;
			//Default to pausing when opening editor
			if (isEditorOpen) {
				cameraPosBeforeEditorWasOpened = GameModule::GetMainCamera()->transform.GetPosition();
				isWorldPaused = true;
				//TODO: Stash timescale and restore it on unpause (both here and in regular pause part)
				Time::SetTimeScale(0.f);
			} else {
				GameModule::GetMainCamera()->FireEvent("SetPosition", {{ "pos", &cameraPosBeforeEditorWasOpened }});
				isWorldPaused = false;
				Time::SetTimeScale(1.f);
			}
		}
		if (isEditorOpen) {
			if (isWorldPaused && Input::GetKey(KC_LALT)) {
				if (io.MouseWheel != 0) {
					cameraDragMultiplier += io.MouseWheel * CAMERA_DRAG_INCREASE_STEP;
					if (cameraDragMultiplier < CAMERA_DRAG_MULTIPLIER_MIN) {
						cameraDragMultiplier = CAMERA_DRAG_MULTIPLIER_MIN;
					} else if (cameraDragMultiplier > CAMERA_DRAG_MULTIPLIER_MAX) {
						cameraDragMultiplier = CAMERA_DRAG_MULTIPLIER_MAX;
					}
				}
				ImGui::SetMouseCursor(ImGuiMouseCursor_Move);
				if (ImGui::IsMouseDragging()) {
					auto dragDelta = ImGui::GetMouseDragDelta();
					ImGui::ResetMouseDragDelta();
					Vec2 delta(-dragDelta.x * cameraDragMultiplier, dragDelta.y * cameraDragMultiplier);
					GameModule::GetMainCamera()->FireEvent("CameraEditorDrag", {{"delta", &delta}});
				}
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
