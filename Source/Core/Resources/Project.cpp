#include "Project.h"

#include "Core/DllManager.h"
#include "Core/Logging/Logger.h"
#include "Core/SceneManager.h"
#include "Core/Serialization/SchemaValidator.h"
#include "Core/physicsworld.h"

static auto const logger = Logger::Create("Project");

REFLECT_STRUCT_BEGIN(Project)
REFLECT_STRUCT_END()

static SerializedObjectSchema const PROJECT_SCHEMA = SerializedObjectSchema(
    "Project", {SerializedPropertySchema::Required("name", SerializedValueType::STRING),
                SerializedPropertySchema::Required("startingScene", SerializedValueType::STRING,
                                                   {SerializedPropertyFlag::IS_FILE_PATH}),
                SerializedPropertySchema::RequiredObject("startingGravity", "Vec3"),
                SerializedPropertySchema::RequiredArray("dlls", SerializedValueType::STRING,
                                                        {SerializedPropertyFlag::IS_FILE_PATH}),
                SerializedPropertySchema::RequiredObjectArray("defaultKeybindings", "Keybinding")});

class ProjectDeserializer : public Deserializer
{

    SerializedObjectSchema GetSchema() override { return PROJECT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) override
    {
        auto proj = Project::Deserialize(ctx, obj);
        if (!proj.has_value()) {
            return nullptr;
        }
        return new Project(proj.value());
    }
};

DESERIALIZABLE_IMPL(Project, new ProjectDeserializer())

std::optional<Project> Project::Deserialize(DeserializationContext * deserializationContext,
                                            SerializedObject const & obj)
{
    auto validationResult = SchemaValidator::Validate(PROJECT_SCHEMA, obj);
    if (!validationResult.isValid) {
        logger->Warnf("Failed to deserialize Project from SerializedObject, validation failed. Errors:");
        for (auto const & err : validationResult.propertyErrors) {
            logger->Warnf("\t%s: %s", err.first, err.second);
        }
        return std::nullopt;
    }
    auto name = obj.GetString("name").value();
    auto startingScene = std::filesystem::path(obj.GetString("startingScene").value());
    auto startingGravityObj = obj.GetObject("startingGravity").value();
    glm::vec3 startingGravity(startingGravityObj.GetNumber("x").value(),
                              startingGravityObj.GetNumber("y").value(),
                              startingGravityObj.GetNumber("z").value());
    auto dllsArr = obj.GetArray("dlls").value();
    std::vector<std::filesystem::path> dlls(dllsArr.size());
    for (size_t i = 0; i < dllsArr.size(); ++i) {
        dlls[i] = std::filesystem::path(std::get<std::string>(dllsArr[i]));
    }
    auto kbArr = obj.GetArray("defaultKeybindings").value();
    std::vector<Keybinding> defaultKeybindings;
    defaultKeybindings.reserve(kbArr.size());
    for (size_t i = 0; i < kbArr.size(); ++i) {
        auto const & kb = std::get<SerializedObject>(kbArr[i]);
        auto const kc = kb.GetArray("keyCodes").value();
        std::vector<Keycode> keyCodes(kc.size());
        for (size_t j = 0; j < kc.size(); ++j) {
            keyCodes[j] = strToKeycode[std::get<std::string>(kc[j])];
        }
        defaultKeybindings.emplace_back(kb.GetString("name").value(), keyCodes);
    }
    return Project(name, startingScene, startingGravity, dlls, defaultKeybindings);
}

Project::Project(std::string name, std::filesystem::path startingScene, glm::vec3 startingGravity,
                 std::vector<std::filesystem::path> dlls, std::vector<Keybinding> defaultKeybindings)
    : name(name), startingScene(startingScene), startingGravity(startingGravity), dlls(dlls),
      defaultKeybindings(defaultKeybindings)
{
    this->type = "Project";
}

SerializedObject Project::Serialize() const
{
    SerializedArray serializedDlls(dlls.size());
    for (size_t i = 0; i < dlls.size(); ++i) {
        serializedDlls[i] = dlls[i].string();
    }
    SerializedArray serializedDefaultKeybindings(defaultKeybindings.size());
    for (size_t i = 0; i < defaultKeybindings.size(); ++i) {
        auto const & kb = defaultKeybindings[i];
        auto const & kc = kb.GetKeyCodes();
        SerializedArray serializedKeyCodes(kc.size());
        for (size_t j = 0; j < kc.size(); ++j) {
            serializedKeyCodes[j] = keycodeToStr[kc[j]];
        }
        serializedDefaultKeybindings[i] = SerializedObject::Builder()
                                              .WithString("name", kb.GetName())
                                              .WithArray("keyCodes", serializedKeyCodes)
                                              .Build();
    }
    return SerializedObject::Builder()
        .WithString("type", "Project")
        .WithString("name", name)
        .WithString("startingScene", startingScene.string())
        .WithObject("startingGravity",
                    SerializedObject::Builder()
                        .WithNumber("x", startingGravity.x)
                        .WithNumber("y", startingGravity.y)
                        .WithNumber("z", startingGravity.z)
                        .Build())
        .WithArray("dlls", serializedDlls)
        .WithArray("defaultKeybindings", serializedDefaultKeybindings)
        .Build();
}

void Project::Load(ProjectLoadContext ctx)
{
    ctx.RemoveAllKeybindings();
    // TODO: This is problematic, but has to be done this way because scene must be unloaded before DLL is unloaded
    ctx.sceneManager->UnloadCurrentScene();
    ctx.dllManager->UnloadAllDlls();
    for (auto const & kb : defaultKeybindings) {
        for (auto const kc : kb.GetKeyCodes()) {
            ctx.AddKeybind(kb.GetName(), kc);
        }
    }
    for (auto const & dll : dlls) {
        if (dll.is_absolute()) {
            ctx.dllManager->LoadDll(dll);
        } else {
            ctx.dllManager->LoadDll(ctx.deserializationContext->workingDirectory / dll);
        }
    }
    ctx.physicsWorld->SetGravity(startingGravity);
    if (startingScene.is_absolute()) {
        ctx.sceneManager->ChangeScene(startingScene);
    } else {
        ctx.sceneManager->ChangeScene(ctx.deserializationContext->workingDirectory / startingScene);
    }
}