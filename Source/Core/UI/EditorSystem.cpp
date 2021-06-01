#include "Core/UI/EditorSystem.h"

#include <cstdio>

#include <imgui.h>

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imfilebrowser.h>
#include <optick/optick.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Components/UneditableComponent.h"
#include "Core/EntityManager.h"
#include "Core/GameModule.h"
#include "Core/Input/Input.h"
#include "Core/Logging/Logger.h"
#include "Core/entity.h"
#define REFLECT_IMPL
#include "Core/Reflect.h"
#undef REFLECT_IMPL
#include "Core/DllManager.h"
#include "Core/ProjectManager.h"
#include "Core/Resources/Scene.h"
#include "Core/SceneManager.h"
#include "Core/Serialization/JsonSerializer.h"
#include "Core/dtime.h"
#include "EditProjectDialog.h"
#include "SerializedObjectEditor.h"
#include "TypeChooser.h"
#include "Util/DefaultFileSlurper.h"
#include "Util/Imgui_InputText.h"
#include "Util/WriteToFile.h"

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
        ImGui::InputFloat("##hidelabel", f.v, 0.f, 0.f, "%.3f", f.extra_flags);
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
    case EditorNode::STRING: {
        auto s = std::move(std::get<EditorNode::String>(e->node));
        ImGui::Text(s.label.c_str());
        ImGui::SameLine();
        ImGui::PushID((int)s.v);
        ImGui_InputText("##hidelabel", s.v);
        ImGui::PopID();
        break;
    }
    default: {
        logger->Warnf("Unknown EditorNode type=%zu", e->type);
    }
    }
}

namespace EditorSystem
{
EntityManager * entityManager;
ProjectManager * projectManager;
SceneManager * sceneManager;

bool isEditorOpen = false;
bool isWorldPaused = false;

bool resetOnPause = true;

const float CAMERA_DRAG_MULTIPLIER_MIN = 5.f;
const float CAMERA_DRAG_MULTIPLIER_MAX = 100.f;
const float CAMERA_DRAG_INCREASE_STEP = 20.f;
float cameraDragMultiplier = 30.f;

EntityPtr editorCamera;

struct ActiveProject {
    std::filesystem::path path;
};

struct ActiveScene {
    std::filesystem::path path;
};

std::optional<ActiveProject> activeProject;
std::optional<ActiveScene> activeScene;

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
std::optional<SerializedObjectSchema> addComponentEditorSchema = {};

// Entity Editor state
bool isEntityEditorOpen = false;
struct {
    EntityPtr currEntity;
} entityEditor;

struct MenuBarResult {
    bool newProjectDialogOpened = false;
    bool openProjectDialogOpened = false;
    bool newSceneDialogOpened = false;
    bool openSceneDialogOpened = false;
    bool newEntityDialogOpened = false;
};

ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

bool isGamepadStateViewerOpen = false;

EditProjectDialog editProjectDialog;

ImGui::FileBrowser openSceneBrowser;
ImGui::FileBrowser openProjectBrowser;

bool DrawEntityEditor();
void DrawGamepadStateViewer();
MenuBarResult DrawMenuBar();
void DrawNewSceneDialog(bool openedThisFrame);
std::filesystem::path GetSceneWorkingDirectory();
void HandleCameraMovement(ImGuiIO & io);
void SaveScene();
void ToNextEntity();
void ToPrevEntity();

void Init()
{
    entityManager = EntityManager::GetInstance();
    projectManager = ProjectManager::GetInstance();
    projectManager->AddChangeListener("EditorSystemActiveProject",
                                      [](std::filesystem::path newPath) { activeProject = {.path = newPath}; });
    sceneManager = SceneManager::GetInstance();
    sceneManager->AddChangeListener("EditorSystemActiveScene", [](SceneChangeEvent e) {
        if (e.type == SceneChangeEvent::Type::SCENE_LOADED && e.newScene.has_value()) {
            activeScene = {.path = e.newScene.value()};
        } else {
            activeScene = std::nullopt;
        }
    });
    addComponentTypeChooser = TypeChooser("Choose type", TypeChooser::COMPONENT_TYPE_FILTER);

    entitySchema = Deserializable::GetSchema("Entity");
    if (!entitySchema.has_value()) {
        logger->Errorf("Could not find the schema for Entity, this should never happen and will cause a crash.");
    }

    PerspectiveCamera camera;
    camera.aspect = 1.77;
    camera.fov = 90;
    camera.zFar = 10000;
    camera.zNear = 0.1;
    editorCamera =
        entityManager->AddEntity(Entity("EditorCamera",
                                        "EditorCamera",
                                        Transform(),
                                        {new CameraComponent(camera, true, false), new UneditableComponent()}));
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
        auto menuBarResult = DrawMenuBar();

        if (isWorldPaused) {
            HandleCameraMovement(io);
        }
        if (io.KeyCtrl && Input::GetKeyDown(KC_n)) {
            menuBarResult.newSceneDialogOpened = true;
        }
        if (io.KeyCtrl && Input::GetKeyDown(KC_s)) {
            SaveScene();
        }

        if (menuBarResult.newProjectDialogOpened) {
            editProjectDialog.OpenNew(std::filesystem::current_path());
        }
        auto newProjectOpt = editProjectDialog.Draw();
        if (newProjectOpt.has_value()) {
            auto newProjectAndPath = newProjectOpt.value();
            logger->Infof("Creating new project with name=%s, path=%s",
                          newProjectAndPath.second.GetName().c_str(),
                          newProjectAndPath.first.c_str());
            auto serializedNewProject = newProjectAndPath.second.Serialize();
            auto serializedNewProjectString = JsonSerializer().Serialize(serializedNewProject);
            std::filesystem::create_directories(newProjectAndPath.first.parent_path());
            WriteToFile(newProjectAndPath.first, serializedNewProjectString);
            projectManager->ChangeProject(newProjectAndPath.first);
        }

        DrawNewSceneDialog(menuBarResult.newSceneDialogOpened);

        if (menuBarResult.openSceneDialogOpened) {
            openSceneBrowser.SetPwd(activeProject.value().path.parent_path());
            openSceneBrowser.Open();
        }
        openSceneBrowser.Display();
        if (openSceneBrowser.HasSelected()) {
            auto newScenePath = openSceneBrowser.GetSelected();
            sceneManager->ChangeScene(newScenePath);
            openSceneBrowser.Close();
        }

        if (menuBarResult.openProjectDialogOpened) {
            openProjectBrowser.SetPwd(std::filesystem::current_path());
            openProjectBrowser.Open();
        }
        openProjectBrowser.Display();
        if (openProjectBrowser.HasSelected()) {
            auto newProjectPath = openProjectBrowser.GetSelected();
            projectManager->ChangeProject(newProjectPath);
            openProjectBrowser.Close();
        }

        bool isSerializedObjectTypeSelectionOpen = DrawEntityEditor();

        if (isSerializedObjectTypeSelectionOpen) {
            addComponentTypeChooser.value().Open();
        }

        bool addComponentEditorOpenedThisFrame = false;
        if (addComponentTypeChooser.value().Draw(&addComponentEditorSchema)) {
            addComponentEditorOpenedThisFrame = true;
        }

        if (addComponentEditorOpenedThisFrame) {
            addComponentEditor.Open(addComponentEditorSchema.value(), GetSceneWorkingDirectory());
        }

        if (addComponentEditorSchema.has_value()) {
            if (addComponentEditor.Draw(&addComponentNewComponent)) {
                DeserializationContext context = {GetSceneWorkingDirectory()};
                auto newComponent = (Component *)Deserializable::Deserialize(&context, addComponentNewComponent);
                if (!newComponent) {
                    logger->Errorf("Failed to deserialize new component");
                    return;
                }
                auto currEntity = entityEditor.currEntity.Get();
                if (!currEntity) {
                    logger->Errorf("Cannot add new component because currEntity is null");
                    return;
                }
                currEntity->AddComponent(newComponent);
            }
        }

        if (menuBarResult.newEntityDialogOpened) {
            newEntityEditor.Open(entitySchema.value(), GetSceneWorkingDirectory());
        }

        if (newEntityEditor.Draw(&addEntityNewEntity)) {
            DeserializationContext context = {GetSceneWorkingDirectory()};

            auto entityOpt = Entity::Deserialize(&context, addEntityNewEntity);
            if (!entityOpt.has_value()) {
                logger->Errorf("Failed to deserialize new entity");
                return;
            }

            auto entityPtr = entityManager->AddEntity(entityOpt.value());
            sceneManager->GetCurrentScene()->AddEntity(entityPtr);
        }

        // TODO: Make this work with transforms that have parents
        auto currEntity = entityEditor.currEntity.Get();
        auto editorCamEntity = editorCamera.Get();
        if (currEntity && editorCamEntity) {
            float mutableMatrix[16];
            memcpy(mutableMatrix, glm::value_ptr(currEntity->GetTransform()->GetLocalToWorld()), 16 * sizeof(float));
            auto cameraComponent = (CameraComponent *)editorCamEntity->GetComponent("CameraComponent");
            ImGuizmo::Manipulate(glm::value_ptr(cameraComponent->GetView()),
                                 glm::value_ptr(cameraComponent->GetProjection()),
                                 currentGizmoOperation,
                                 ImGuizmo::MODE::WORLD,
                                 mutableMatrix);

            auto newLtw = glm::make_mat4(mutableMatrix);
            glm::vec3 newPos, newScale, ignoredSkew;
            glm::vec4 ignoredPerspective;
            glm::quat newRot;
            glm::decompose(newLtw, newScale, newRot, newPos, ignoredSkew, ignoredPerspective);
            currEntity->GetTransform()->SetPosition(newPos);
            currEntity->GetTransform()->SetRotation(newRot);
            currEntity->GetTransform()->SetScale(newScale);
        }

        DrawGamepadStateViewer();
    }
}

void HandleCameraMovement(ImGuiIO & io)
{
    auto editorCamEntity = editorCamera.Get();
    auto cameraPos = editorCamEntity->GetTransform()->GetPosition();
    auto cameraRot = editorCamEntity->GetTransform()->GetRotation();
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
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            auto dragDelta = ImGui::GetMouseDragDelta();
            ImGui::ResetMouseDragDelta();
            glm::vec2 delta(-dragDelta.x * cameraDragMultiplier, dragDelta.y * cameraDragMultiplier);
            delta *= Time::GetUnscaledDeltaTime();
            cameraPos.x += delta.x;
            cameraPos.y += delta.y;
        }
    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        auto dragDelta = ImGui::GetMouseDragDelta(1);
        ImGui::ResetMouseDragDelta(1);

        cameraRot = glm::rotate(glm::mat4(1.f), dragDelta.x * Time::GetUnscaledDeltaTime(), glm::vec3(0.f, -1.f, 0.f)) *
                    glm::mat4_cast(cameraRot);
        cameraRot = glm::mat4_cast(cameraRot) *
                    glm::rotate(glm::mat4(1.f), dragDelta.y * Time::GetUnscaledDeltaTime(), glm::vec3(-1.f, 0.f, 0.f));
    }

    if (!io.KeyCtrl && !io.KeyShift && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        if (Input::GetKey(KC_w)) {
            auto fwd = editorCamEntity->GetTransform()->GetLocalToWorld() * glm::vec4(0.f, 0.f, -1.f, 0.f);
            fwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
            cameraPos.x += fwd.x;
            cameraPos.y += fwd.y;
            cameraPos.z += fwd.z;
        } else if (Input::GetKey(KC_s)) {
            auto bwd = editorCamEntity->GetTransform()->GetLocalToWorld() * glm::vec4(0.f, 0.f, 1.f, 0.f);
            bwd *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
            cameraPos.x += bwd.x;
            cameraPos.y += bwd.y;
            cameraPos.z += bwd.z;
        }

        if (Input::GetKey(KC_a)) {
            auto left = editorCamEntity->GetTransform()->GetLocalToWorld() * glm::vec4(-1.f, 0.f, 0.f, 0.f);
            left *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
            cameraPos.x += left.x;
            cameraPos.y += left.y;
            cameraPos.z += left.z;
        } else if (Input::GetKey(KC_d)) {
            auto right = editorCamEntity->GetTransform()->GetLocalToWorld() * glm::vec4(1.f, 0.f, 0.f, 0.f);
            right *= cameraDragMultiplier * Time::GetUnscaledDeltaTime();
            cameraPos.x += right.x;
            cameraPos.y += right.y;
            cameraPos.z += right.z;
        }
    }

    editorCamEntity->GetTransform()->SetPosition(cameraPos);
    editorCamEntity->GetTransform()->SetRotation(cameraRot);
}

MenuBarResult DrawMenuBar()
{
    MenuBarResult result;
    if (ImGui::BeginMainMenuBar()) {
        ImGui::Columns(2);
        auto tempSceneLocation = (GetSceneWorkingDirectory() / "_editor.scene").string();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                result.newProjectDialogOpened = true;
            }
            if (ImGui::MenuItem("Open Project")) {
                result.openProjectDialogOpened = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Scene", nullptr, nullptr, activeProject.has_value())) {
                result.newSceneDialogOpened = true;
            }
            if (ImGui::MenuItem("Open Scene", nullptr, nullptr, activeProject.has_value())) {
                result.openSceneDialogOpened = true;
            }
            if (ImGui::MenuItem("Save Scene", nullptr, nullptr, activeScene.has_value())) {
                SaveScene();
            }
            if (ImGui::MenuItem("Reload Scene", nullptr, nullptr, activeScene.has_value())) {
                sceneManager->ChangeScene(activeScene.value().path);
            }
            if (ImGui::MenuItem("Unload Scene", nullptr, nullptr, activeScene.has_value())) {
                sceneManager->UnloadCurrentScene();
            }
            ImGui::Separator();
            ImGui::MenuItem("Reset on pause", nullptr, &resetOnPause);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Entities")) {
            if (ImGui::MenuItem("Entity Editor", nullptr, &isEntityEditorOpen, activeScene.has_value())) {
                if (!entityEditor.currEntity) {
                    entityEditor.currEntity = entityManager->First();
                }
            }
            if (ImGui::MenuItem("New Entity", nullptr, nullptr, activeScene.has_value())) {
                addEntityNewEntity = SerializedObject();
                result.newEntityDialogOpened = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Utilities")) {
            ImGui::MenuItem("Gamepad state viewer", nullptr, &isGamepadStateViewerOpen);
            ImGui::EndMenu();
        }
        if (isWorldPaused && ImGui::MenuItem("Play", nullptr, nullptr, activeScene.has_value())) {
            Play();
        } else if (!isWorldPaused && ImGui::MenuItem("Pause", nullptr, nullptr, activeScene.has_value())) {
            Pause(resetOnPause);
        }

        if (editorCamera) {
            auto camPos = editorCamera.Get()->GetTransform()->GetPosition();
            ImGui::Text("x: %f y: %f z: %f", camPos.x, camPos.y, camPos.z);
        }

        ImGui::NextColumn();
        // Hack for right alignment
        float rightAlignedItemsSize = ImGui::CalcTextSize("Toggle Camera").x + ImGui::CalcTextSize("T").x +
                                      ImGui::CalcTextSize("R").x + ImGui::CalcTextSize("S").x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - rightAlignedItemsSize -
                             ImGui::GetScrollX() - 8 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::MenuItem("Toggle Camera")) {
            auto mainCamera = entityManager->GetEntityBySingletonTag(EntityManager::IS_MAIN_CAMERA_TAG);
            Entity * mainCameraEntity = nullptr;
            if (mainCamera.has_value()) {
                mainCameraEntity = mainCamera.value().Get();
            }
            auto editorCamEntity = editorCamera.Get();
            if (editorCamEntity) {
                auto editorCameraComponent = (CameraComponent *)editorCamEntity->GetComponent("CameraComponent");
                if (editorCameraComponent->IsActive()) {
                    editorCameraComponent->SetActive(false);
                    if (mainCameraEntity) {
                        ((CameraComponent *)mainCameraEntity->GetComponent("CameraComponent"))->SetActive(true);
                    }
                } else {
                    editorCameraComponent->SetActive(true);
                    if (mainCameraEntity) {
                        ((CameraComponent *)mainCameraEntity->GetComponent("CameraComponent"))->SetActive(false);
                    }
                }
            }
        }
        if (ImGui::MenuItem("T")) {
            currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (ImGui::MenuItem("R")) {
            currentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
        }
        if (ImGui::MenuItem("S")) {
            currentGizmoOperation = ImGuizmo::OPERATION::SCALE;
        }

        ImGui::EndMainMenuBar();
    }
    return result;
}

bool DrawEntityEditor()
{
    auto currEntity = entityEditor.currEntity.Get();
    bool openTypeSelection = false;
    if (isEntityEditorOpen && ImGui::Begin("Entity Editor", &isEntityEditorOpen)) {
        if (ImGui::SmallButton("Prev")) {
            ToPrevEntity();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Next")) {
            ToNextEntity();
        }
        if (currEntity != nullptr && !currEntity->HasComponent("UneditableComponent")) {
            ImGui::Text(currEntity->GetName().c_str());
            ImGui::Separator();
            auto eDesc = reflect::TypeResolver<Entity>::get(currEntity);
            auto e = eDesc->DrawEditorGui("Entity", currEntity, false);
            DrawEditorNode(e.get());
            if (ImGui::Button("Add Component")) {
                openTypeSelection = true;
            }
            ImGui::Separator();
            if (ImGui::Button("Delete entity")) {
                auto newSelectedEntity = entityManager->Prev(entityEditor.currEntity);
                entityManager->RemoveEntity(entityEditor.currEntity);
                sceneManager->GetCurrentScene()->RemoveEntity(entityEditor.currEntity);
                entityEditor.currEntity = newSelectedEntity;
            }
        }
        ImGui::End();
    }
    return openTypeSelection;
}

void DrawGamepadStateViewer()
{
    if (isGamepadStateViewerOpen && ImGui::Begin("Gamepad state viewer", &isGamepadStateViewerOpen)) {
        auto gamepadCount = Input::GetGamepadCount();
        for (int i = 0; i < gamepadCount; ++i) {
            auto pad = Input::GetGamepad(i);
            if (pad &&
                ImGui::TreeNodeEx(std::to_string(i).c_str(), gamepadCount == 1 ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
                ImGui::Text(
                    "LX: %f (%f)", pad->GetAxis(GamepadAxis::AXIS_LEFTX), pad->GetAxisRaw(GamepadAxis::AXIS_LEFTX));
                ImGui::Text(
                    "LY: %f (%f)", pad->GetAxis(GamepadAxis::AXIS_LEFTY), pad->GetAxisRaw(GamepadAxis::AXIS_LEFTY));
                ImGui::Text("LT: %f (%f)",
                            pad->GetAxis(GamepadAxis::AXIS_TRIGGERLEFT),
                            pad->GetAxisRaw(GamepadAxis::AXIS_TRIGGERLEFT));
                ImGui::Value("LB", pad->GetButton(GamepadButton::BUTTON_LEFTSHOULDER));
                ImGui::Value("LS", pad->GetButton(GamepadButton::BUTTON_LEFTSTICK));
                ImGui::Value("Down", pad->GetButton(GamepadButton::BUTTON_DPAD_DOWN));
                ImGui::Value("Left", pad->GetButton(GamepadButton::BUTTON_DPAD_LEFT));
                ImGui::Value("Up", pad->GetButton(GamepadButton::BUTTON_DPAD_UP));
                ImGui::Value("Right", pad->GetButton(GamepadButton::BUTTON_DPAD_RIGHT));

                ImGui::Value("Back", pad->GetButton(GamepadButton::BUTTON_BACK));
                ImGui::Value("Guide", pad->GetButton(GamepadButton::BUTTON_GUIDE));
                ImGui::Value("Start", pad->GetButton(GamepadButton::BUTTON_START));

                ImGui::Text(
                    "RX: %f (%f)", pad->GetAxis(GamepadAxis::AXIS_RIGHTX), pad->GetAxisRaw(GamepadAxis::AXIS_RIGHTX));
                ImGui::Text(
                    "RY: %f (%f)", pad->GetAxis(GamepadAxis::AXIS_RIGHTY), pad->GetAxisRaw(GamepadAxis::AXIS_RIGHTY));
                ImGui::Text("RT: %f (%f)",
                            pad->GetAxis(GamepadAxis::AXIS_TRIGGERRIGHT),
                            pad->GetAxisRaw(GamepadAxis::AXIS_TRIGGERRIGHT));
                ImGui::Value("RX", pad->GetAxis(GamepadAxis::AXIS_RIGHTX), "%f");
                ImGui::Value("RY", pad->GetAxis(GamepadAxis::AXIS_RIGHTY), "%f");
                ImGui::Value("RT", pad->GetAxis(GamepadAxis::AXIS_TRIGGERRIGHT), "%f");
                ImGui::Value("RB", pad->GetButton(GamepadButton::BUTTON_RIGHTSHOULDER));
                ImGui::Value("RS", pad->GetButton(GamepadButton::BUTTON_RIGHTSTICK));
                ImGui::Value("A", pad->GetButton(GamepadButton::BUTTON_A));
                ImGui::Value("X", pad->GetButton(GamepadButton::BUTTON_X));
                ImGui::Value("Y", pad->GetButton(GamepadButton::BUTTON_Y));
                ImGui::Value("B", pad->GetButton(GamepadButton::BUTTON_B));

                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
}

void DrawNewSceneDialog(bool openedThisFrame)
{
    if (openedThisFrame) {
        ImGui::OpenPopup("New Scene");
    }
    if (ImGui::BeginPopupModal("New Scene")) {
        ImGui::SetWindowSize("New Scene", ImVec2(300, 100));
        auto hasInputtedText = ImGui::InputText("File name", newSceneFilename, newSceneFilenameLen);
        if (openedThisFrame) {
            ImGui::SetItemDefaultFocus();
            ImGui::SetKeyboardFocusHere(-1);
        }
        if (ImGui::Button("OK")) {
            auto parent = std::filesystem::path(newSceneFilename).parent_path();
            std::filesystem::create_directories(parent);
            auto newScene = Scene::CreateNew(newSceneFilename, DeserializationContext{.workingDirectory = parent});
            newScene->SerializeToFile(newSceneFilename);

            auto newSceneToLoad = Scene::FromFile(newSceneFilename, entityManager);
            newSceneToLoad->Load(entityManager);
            entityEditor.currEntity = entityManager->First();

            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CloseEditor()
{
    if (!isEditorOpen) {
        return;
    }
    isEditorOpen = false;
    auto mainCamera = entityManager->GetEntityBySingletonTag(EntityManager::IS_MAIN_CAMERA_TAG);
    if (mainCamera.has_value()) {
        auto mainCamEntity = mainCamera.value().Get();
        if (mainCamEntity) {
            ((CameraComponent *)mainCamEntity->GetComponent("CameraComponent"))->SetActive(true);
        }
    }
    auto editorCamEntity = editorCamera.Get();
    if (editorCamEntity) {
        ((CameraComponent *)editorCamEntity->GetComponent("CameraComponent"))->SetActive(false);
    }
    Play();
}

void OpenEditor()
{
    if (isEditorOpen) {
        return;
    }
    isEditorOpen = true;
    auto mainCamera = entityManager->GetEntityBySingletonTag(EntityManager::IS_MAIN_CAMERA_TAG);
    if (mainCamera.has_value()) {
        auto mainCamEntity = mainCamera.value().Get();
        if (mainCamEntity) {
            ((CameraComponent *)mainCamEntity->GetComponent("CameraComponent"))->SetActive(false);
        }
    }
    auto editorCamEntity = editorCamera.Get();
    if (editorCamEntity) {
        ((CameraComponent *)editorCamEntity->GetComponent("CameraComponent"))->SetActive(true);
    }
    Pause(false);
}

void Pause(bool reset)
{
    auto tempSceneLocation = (GetSceneWorkingDirectory() / "_editor.scene");
    Time::SetTimeScale(0.f);
    isWorldPaused = true;
    if (reset) {
        logger->Infof("Reset on pause enabled, reloading scene");
        std::optional<EntityId> currEntityId;
        if (entityEditor.currEntity) {
            currEntityId = entityEditor.currEntity.Get()->GetId();
        }
        sceneManager->ChangeScene(tempSceneLocation);
        if (currEntityId.has_value()) {
            entityEditor.currEntity = entityManager->GetEntityById(currEntityId.value());
        }
        if (!entityEditor.currEntity) {
            entityEditor.currEntity = entityManager->First();
        }
        logger->Infof("Finished reloading scene");
    }
}

void Play()
{
    auto tempSceneLocation = (GetSceneWorkingDirectory() / "_editor.scene").string();
    Time::SetTimeScale(1.f);
    isWorldPaused = false;
    sceneManager->GetCurrentScene()->SerializeToFile(tempSceneLocation);
}

void ToNextEntity()
{
    auto startEntity = entityEditor.currEntity;
    entityEditor.currEntity = entityManager->Next(entityEditor.currEntity);
    while ((!entityEditor.currEntity || entityEditor.currEntity.Get()->HasComponent("UneditableComponent")) &&
           entityEditor.currEntity != startEntity && entityEditor.currEntity) {
        entityEditor.currEntity = entityManager->Next(entityEditor.currEntity);
    }
}

void ToPrevEntity()
{
    auto startEntity = entityEditor.currEntity;
    entityEditor.currEntity = entityManager->Prev(entityEditor.currEntity);
    while ((!entityEditor.currEntity || entityEditor.currEntity.Get()->HasComponent("UneditableComponent")) &&
           entityEditor.currEntity != startEntity && entityEditor.currEntity) {
        entityEditor.currEntity = entityManager->Prev(entityEditor.currEntity);
    }
}

std::filesystem::path GetSceneWorkingDirectory()
{
    if (activeScene.has_value()) {
        return activeScene.value().path.parent_path();
    }
    return std::filesystem::current_path();
}

void SaveScene()
{
    // Always save main camera as active, otherwise it may be saved as inactive if the editor camera is active
    auto mainCamera = entityManager->GetEntityBySingletonTag(EntityManager::IS_MAIN_CAMERA_TAG);
    if (mainCamera.has_value()) {
        auto mainCamEntity = mainCamera.value().Get();
        if (mainCamEntity) {
            ((CameraComponent *)mainCamEntity->GetComponent("CameraComponent"))->SetActive(true);
        }
    }
    sceneManager->GetCurrentScene()->SerializeToFile(activeScene.value().path.string());
}
}
