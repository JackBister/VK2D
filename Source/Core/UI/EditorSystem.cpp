#include "Core/UI/EditorSystem.h"

#include <cstdio>
#include <imgui.h>
#include <optick/optick.h>

#include "Core/Components/CameraComponent.h"
#include "Core/GameModule.h"
#include "Core/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/entity.h"
#define REFLECT_IMPL
#include "Core/Reflect.h"
#undef REFLECT_IMPL
#include "Core/Resources/Scene.h"
#include "Core/dtime.h"
#include "SerializedObjectEditor.h"
#include "TypeChooser.h"

static const auto logger = Logger::Create("EditorSystem");

void SaveScene();

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

bool resetOnPause = true;

const float CAMERA_DRAG_MULTIPLIER_MIN = 5.f;
const float CAMERA_DRAG_MULTIPLIER_MAX = 100.f;
const float CAMERA_DRAG_INCREASE_STEP = 20.f;
float cameraDragMultiplier = 30.f;

Entity * editorCamera;

// Scene menu state
char newSceneFilename[256];
size_t newSceneFilenameLen = sizeof(newSceneFilename);

std::optional<TypeChooser> addComponentTypeChooser;
SerializedObject addComponentNewComponent;
SerializedObjectEditor addComponentEditor("Add component");

SerializedObject addEntityNewEntity;
SerializedObjectEditor newEntityEditor("Add entity");
std::optional<SerializedObjectSchema> entitySchema;

bool isSerializedObjectEditorOpen = false;
std::optional<SerializedObjectSchema> serializedObjectEditorSchema = {};

// Entity Editor state
bool isEntityEditorOpen = false;
struct {
    Entity * currEntity = nullptr;
    size_t currEntityIndex = 0;
} entityEditor;

void Init()
{
    addComponentTypeChooser = TypeChooser("Choose type");

    entitySchema = Deserializable::GetSchema("Entity");
    if (!entitySchema.has_value()) {
        logger->Errorf("Could not find the schema for Entity, this should never happen and will cause a crash.");
    }

    editorCamera = new Entity();
    editorCamera->name = "EditorCamera";
    editorCamera->type = "Entity";
    PerspectiveCamera camera;
    camera.aspect = 1.77;
    camera.fov = 90;
    camera.zFar = 10000;
    camera.zNear = 0.1;
    editorCamera->AddComponent(std::make_unique<CameraComponent>(camera));
    GameModule::AddEntity(editorCamera);
}

void OnGui()
{
    OPTICK_EVENT();
    auto io = ImGui::GetIO();
    if (Input::GetKeyDown(KC_F7)) {
        if (isEditorOpen) {
            CloseEditor();
        } else {
            OpenEditor();
        }
    }
    if (isEditorOpen) {
        if (isWorldPaused) {
            auto cameraPos = editorCamera->transform.GetPosition();
            auto cameraRot = editorCamera->transform.GetRotation();
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

            if (!io.KeyCtrl && !io.KeyShift && !ImGui::IsAnyWindowFocused()) {
                if (Input::GetKey(KC_w)) {
                    auto fwd = editorCamera->transform.GetLocalToWorld() * glm::vec4(0.f, 0.f, -1.f, 0.f);
                    fwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                    cameraPos.x += fwd.x;
                    cameraPos.y += fwd.y;
                    cameraPos.z += fwd.z;
                } else if (Input::GetKey(KC_s)) {
                    auto bwd = editorCamera->transform.GetLocalToWorld() * glm::vec4(0.f, 0.f, 1.f, 0.f);
                    bwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                    cameraPos.x += bwd.x;
                    cameraPos.y += bwd.y;
                    cameraPos.z += bwd.z;
                }

                if (Input::GetKey(KC_a)) {
                    auto left = editorCamera->transform.GetLocalToWorld() * glm::vec4(-1.f, 0.f, 0.f, 0.f);
                    left *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                    cameraPos.x += left.x;
                    cameraPos.y += left.y;
                    cameraPos.z += left.z;
                } else if (Input::GetKey(KC_d)) {
                    auto right = editorCamera->transform.GetLocalToWorld() * glm::vec4(1.f, 0.f, 0.f, 0.f);
                    right *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
                    cameraPos.x += right.x;
                    cameraPos.y += right.y;
                    cameraPos.z += right.z;
                }
            }

            editorCamera->transform.SetPosition(cameraPos);
            editorCamera->transform.SetRotation(cameraRot);
        }
        bool addEntityDialogOpened = false;
        bool newSceneDialogOpened = false;
        if (io.KeyCtrl && Input::GetKeyDown(KC_n)) {
            newSceneDialogOpened = true;
        }
        if (io.KeyCtrl && Input::GetKeyDown(KC_s)) {
            SaveScene();
        }
        if (ImGui::BeginMainMenuBar()) {
            ImGui::Columns(3);
            auto scene = GameModule::GetScene();
            auto tempSceneLocation =
                (std::filesystem::path(scene->GetFileName()).parent_path() / "_editor.scene").string();
            if (ImGui::BeginMenu("Scene")) {
                if (ImGui::MenuItem("New Scene")) {
                    newSceneDialogOpened = true;
                }
                if (ImGui::MenuItem("Save Scene")) {
                    SaveScene();
                }
                ImGui::MenuItem("Reset on pause", nullptr, &resetOnPause);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Entities")) {
                if (ImGui::MenuItem("Entity Editor", nullptr, &isEntityEditorOpen)) {
                    entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
                }
                if (ImGui::MenuItem("New Entity")) {
                    addEntityNewEntity = SerializedObject();
                    addEntityDialogOpened = true;
                }
                ImGui::EndMenu();
            }
            if (isWorldPaused && ImGui::MenuItem("Play")) {
                Play();
            } else if (!isWorldPaused && ImGui::MenuItem("Pause")) {
                Pause(resetOnPause);
            }

            ImGui::NextColumn();
            ImGui::NextColumn();
            // Hack for right alignment
            float rightAlignedItemsSize = ImGui::CalcTextSize("Toggle Camera").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - rightAlignedItemsSize -
                                 ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::MenuItem("Toggle Camera")) {
                auto editorCameraComponent = (CameraComponent *)editorCamera->GetComponent("CameraComponent");
                if (editorCameraComponent->IsActive()) {
                    editorCameraComponent->SetActive(false);
                    ((CameraComponent *)GameModule::GetMainCamera()->GetComponent("CameraComponent"))->SetActive(true);
                } else {
                    editorCameraComponent->SetActive(true);
                    ((CameraComponent *)GameModule::GetMainCamera()->GetComponent("CameraComponent"))->SetActive(false);
                }
            }

            ImGui::EndMainMenuBar();
        }

        if (newSceneDialogOpened) {
            ImGui::OpenPopup("New Scene");
        }
        if (ImGui::BeginPopupModal("New Scene")) {
            ImGui::SetWindowSize("New Scene", ImVec2(300, 100));
            auto hasInputtedText = ImGui::InputText("File name", newSceneFilename, newSceneFilenameLen);
            if (newSceneDialogOpened) {
                ImGui::SetItemDefaultFocus();
                ImGui::SetKeyboardFocusHere(-1);
            }
            if (ImGui::Button("OK")) {
                auto parent = std::filesystem::path(newSceneFilename).parent_path();
                std::filesystem::create_directories(parent);
                auto newScene = Scene::Create(newSceneFilename);
                newScene->SerializeToFile(newSceneFilename);

                GameModule::LoadScene(newSceneFilename);
                entityEditor.currEntityIndex = 0;
                entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        bool isSerializedObjectTypeSelectionOpen = false;
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
                if (ImGui::Button("Add Component")) {
                    isSerializedObjectTypeSelectionOpen = true;
                }
                ImGui::Separator();
                if (ImGui::Button("Delete entity")) {
                    GameModule::RemoveEntity(entityEditor.currEntity);
                    GameModule::GetScene()->RemoveEntity(entityEditor.currEntity);
                    // TODO: This only happens to work right now because Entities are new-ed, in the future they may be
                    // allocated in another way
                    delete entityEditor.currEntity;
                    if (entityEditor.currEntityIndex > 0) {
                        entityEditor.currEntityIndex--;
                    }
                    entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
                }
            }
            ImGui::End();
        }

        if (isSerializedObjectTypeSelectionOpen) {
            addComponentTypeChooser.value().Open();
        }

        bool serializedObjectEditorOpenedThisFrame = false;
        if (addComponentTypeChooser.value().Draw(&serializedObjectEditorSchema)) {
            serializedObjectEditorOpenedThisFrame = true;
        }

        if (serializedObjectEditorOpenedThisFrame) {
            addComponentEditor.Open(serializedObjectEditorSchema.value());
        }

        if (serializedObjectEditorSchema.has_value()) {
            if (addComponentEditor.Draw(&addComponentNewComponent)) {
                DeserializationContext context = {
                    std::filesystem::path(GameModule::GetScene()->GetFileName()).parent_path()};
                auto newComponent = (Component *)Deserializable::Deserialize(&context, addComponentNewComponent);
                if (!newComponent) {
                    logger->Errorf("Failed to deserialize new component");
                    return;
                }
                if (!entityEditor.currEntity) {
                    logger->Errorf("Cannot add new component because currEntity is null");
                    return;
                }
                newComponent->entity = entityEditor.currEntity;
                entityEditor.currEntity->components.push_back(std::unique_ptr<Component>(newComponent));
            }
        }

        if (addEntityDialogOpened) {
            newEntityEditor.Open(entitySchema.value());
        }

        if (newEntityEditor.Draw(&addEntityNewEntity)) {
            DeserializationContext context = {
                std::filesystem::path(GameModule::GetScene()->GetFileName()).parent_path()};

            auto entity = (Entity *)Deserializable::Deserialize(&context, addEntityNewEntity);
            if (!entity) {
                logger->Errorf("Failed to deserialize new entity");
                return;
            }

            GameModule::AddEntity(entity);
            GameModule::GetScene()->AddEntity(entity);
        }
    }
}

void CloseEditor()
{
    if (!isEditorOpen) {
        return;
    }
    isEditorOpen = false;
    ((CameraComponent *)GameModule::GetMainCamera()->GetComponent("CameraComponent"))->SetActive(true);
    ((CameraComponent *)editorCamera->GetComponent("CameraComponent"))->SetActive(false);
    Play();
}

void OpenEditor()
{
    if (isEditorOpen) {
        return;
    }
    isEditorOpen = true;
    ((CameraComponent *)GameModule::GetMainCamera()->GetComponent("CameraComponent"))->SetActive(false);
    ((CameraComponent *)editorCamera->GetComponent("CameraComponent"))->SetActive(true);
    Pause(false);
}

void Pause(bool reset)
{
    auto scene = GameModule::GetScene();
    auto tempSceneLocation = (std::filesystem::path(scene->GetFileName()).parent_path() / "_editor.scene").string();
    Time::SetTimeScale(0.f);
    isWorldPaused = true;
    if (reset) {
        logger->Infof("Reset on pause enabled, reloading scene");
        GameModule::LoadScene(tempSceneLocation);
        entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
        if (!entityEditor.currEntity) {
            entityEditor.currEntityIndex = GameModule::GetEntityCount() - 1;
            entityEditor.currEntity = GameModule::GetEntityByIdx(entityEditor.currEntityIndex);
        }
        logger->Infof("Finished reloading scene");
    }
}

void Play()
{
    auto scene = GameModule::GetScene();
    auto tempSceneLocation = (std::filesystem::path(scene->GetFileName()).parent_path() / "_editor.scene").string();
    Time::SetTimeScale(1.f);
    isWorldPaused = false;
    scene->SerializeToFile(tempSceneLocation);
}
}

void SaveScene()
{
    auto scene = GameModule::GetScene();
    // Always save main camera as active, otherwise it may be saved as inactive if the editor camera is active
    ((CameraComponent *)GameModule::GetMainCamera()->GetComponent("CameraComponent"))->SetActive(true);
    scene->SerializeToFile(scene->GetFileName());
}