#include "ParticleEmitterComponent.h"

#include <ThirdParty/optick/src/optick.h>

#include "Core/Rendering/Particles/ParticleSystem.h"
#include "Core/Rendering/PreRenderCommands.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/entity.h"
#include "Logging/Logger.h"
#include "Serialization/DeserializationContext.h"

static const auto logger = Logger::Create("ParticleEmitterComponent");

REFLECT_STRUCT_BEGIN(ParticleEmitterComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const PARTICLE_EMITTER_COMPONENT_SCHEMA =
    SerializedObjectSchema("ParticleEmitterComponent",
                           {
                               SerializedPropertySchema::Required("isActive", SerializedValueType::BOOL),
                               SerializedPropertySchema::Required("isWorldSpace", SerializedValueType::BOOL),
                               SerializedPropertySchema::Required("numParticles", SerializedValueType::DOUBLE),

                               SerializedPropertySchema::Required("emissionRate", SerializedValueType::DOUBLE),

                               SerializedPropertySchema::Required("lifetime", SerializedValueType::DOUBLE),

                               SerializedPropertySchema::OptionalObject("initialVelocityMin", "Vec3"),
                               SerializedPropertySchema::OptionalObject("initialVelocityMax", "Vec3"),
                               SerializedPropertySchema::OptionalObjectArray("accelerators", "Vec3"),

                               SerializedPropertySchema::OptionalObject("initialSizeMin", "Vec2"),
                               SerializedPropertySchema::OptionalObject("initialSizeMax", "Vec2"),
                               SerializedPropertySchema::Optional("alwaysSquareSize", SerializedValueType::BOOL),

                               SerializedPropertySchema::Required("image", SerializedValueType::STRING,
                                                                  SerializedPropertyFlags({IsFilePathFlag()})),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

static glm::vec2 DeserializeVec2(SerializedObject const & obj)
{
    return glm::vec2(obj.GetNumber("x").value(), obj.GetNumber("y").value());
}

static std::optional<glm::vec2> DeserializeVec2Optional(std::optional<SerializedObject> objOpt)
{
    if (!objOpt.has_value()) {
        return std::nullopt;
    }
    return DeserializeVec2(objOpt.value());
}

static SerializedObject SerializeVec2(glm::vec2 v)
{
    return SerializedObject::Builder().WithNumber("x", v.x).WithNumber("y", v.y).Build();
}

static glm::vec3 DeserializeVec3(SerializedObject const & obj)
{
    return glm::vec3(obj.GetNumber("x").value(), obj.GetNumber("y").value(), obj.GetNumber("z").value());
}

static std::optional<glm::vec3> DeserializeVec3Optional(std::optional<SerializedObject> objOpt)
{
    if (!objOpt.has_value()) {
        return std::nullopt;
    }
    return DeserializeVec3(objOpt.value());
}

static SerializedObject SerializeVec3(glm::vec3 v)
{
    return SerializedObject::Builder().WithNumber("x", v.x).WithNumber("y", v.y).WithNumber("z", v.z).Build();
}

class ParticleEmitterComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return PARTICLE_EMITTER_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto particleSystem = ParticleSystem::GetInstance();
        bool isActive = obj.GetBool("isActive").value();
        bool isWorldSpace = obj.GetBool("isWorldSpace").value();
        uint32_t numParticles = obj.GetNumber("numParticles").value();

        float emissionRate = obj.GetNumber("emissionRate").value();

        float lifetime = obj.GetNumber("lifetime").value();

        glm::vec3 initialVelocityMin =
            DeserializeVec3Optional(obj.GetObject("initialVelocityMin")).value_or(glm::vec3(0.f));
        glm::vec3 initialVelocityMax =
            DeserializeVec3Optional(obj.GetObject("initialVelocityMax")).value_or(glm::vec3(0.f));
        std::vector<glm::vec3> accelerators;
        auto acceleratorsOpt = obj.GetArray("accelerators");
        if (acceleratorsOpt.has_value()) {
            auto acceleratorsArr = acceleratorsOpt.value();
            for (auto const & acceleratorVal : acceleratorsArr) {
                auto acceleratorObj = std::get<SerializedObject>(acceleratorVal);
                accelerators.push_back(DeserializeVec3(acceleratorObj));
            }
        }

        glm::vec2 initialSizeMin = DeserializeVec2Optional(obj.GetObject("initialSizeMin")).value_or(glm::vec2(1.f));
        glm::vec2 initialSizeMax = DeserializeVec2Optional(obj.GetObject("initialSizeMax")).value_or(glm::vec2(1.f));
        bool alwaysSquareSize = obj.GetBool("alwaysSquareSize").value_or(true);

        std::string imagePath = obj.GetString("image").value();
        auto fullPath = ctx->workingDirectory / imagePath;

        auto img = Image::FromFile(fullPath.string());
        if (img == nullptr) {
            logger.Error("Failed to get image with path={}", imagePath);
            return nullptr;
        }

        ParticleEmitter emitter(isActive,
                                isWorldSpace,
                                numParticles,
                                true,
                                emissionRate,
                                lifetime,
                                initialVelocityMin,
                                initialVelocityMax,
                                accelerators,
                                initialSizeMin,
                                initialSizeMax,
                                alwaysSquareSize,
                                img->GetDefaultView());
        auto particleEmitterIdOpt = particleSystem->AddEmitter(emitter);
        if (!particleEmitterIdOpt.has_value()) {
            logger.Error("Failed to create emitter. There is probably logging above explaining why.");
            return nullptr;
        }
        return new ParticleEmitterComponent(particleSystem, particleEmitterIdOpt.value(), imagePath);
    }
};

COMPONENT_IMPL(ParticleEmitterComponent, new ParticleEmitterComponentDeserializer());

ParticleEmitterComponent::~ParticleEmitterComponent()
{
    particleSystem->RemoveEmitter(particleEmitterId);
}

ParticleEmitterComponent::ParticleEmitterComponent(ParticleSystem * particleSystem, ParticleEmitterId particleEmitterId,
                                                   std::string const & imageFilename)
    : particleSystem(particleSystem), particleEmitterId(particleEmitterId), imageFilename(imageFilename)
{
}

SerializedObject ParticleEmitterComponent::Serialize() const
{
    auto emitterOpt = particleSystem->GetEmitter(particleEmitterId);
    if (!emitterOpt.has_value()) {
        std::string entityName;
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            entityName = "";
        } else {
            entityName = e->GetName();
        }
        logger.Error(
            "Did not find emitter with id={} when serializing ParticleEmitterComponent attached to entity with name={}",
            particleEmitterId,
            entityName);
    }

    auto builder = SerializedObject::Builder().WithString("type", this->Reflection.name);
    if (emitterOpt.has_value()) {
        auto emitter = emitterOpt.value();

        SerializedArray accelerators;
        for (auto const & a : emitter.GetAccelerators()) {
            accelerators.push_back(SerializeVec3(a));
        }

        builder = builder.WithBool("isActive", emitter.IsActive())
                      .WithBool("isWorldSpace", emitter.IsWorldSpace())
                      .WithNumber("numParticles", emitter.GetNumParticles())

                      .WithNumber("emissionRate", emitter.GetEmissionRate())

                      .WithNumber("lifetime", emitter.GetLifetime())

                      .WithObject("initialVelocityMin", SerializeVec3(emitter.GetInitialVelocityMin()))
                      .WithObject("initialVelocityMax", SerializeVec3(emitter.GetInitialVelocityMax()))
                      .WithArray("accelerators", accelerators)

                      .WithObject("initialSizeMin", SerializeVec2(emitter.GetInitialSizeMin()))
                      .WithObject("initialSizeMax", SerializeVec2(emitter.GetInitialSizeMax()))
                      .WithBool("alwaysSquareSize", emitter.IsAlwaysSquareSize())

                      .WithString("image", imageFilename);
    }
    return builder.Build();
}

void ParticleEmitterComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif

    if (name == "PreRender") {
        auto builder = (PreRenderCommands::Builder *)args.at("commandBuilder").asPointer;
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        builder->WithParticleEmitterUpdate(
            {.id = particleEmitterId, .localToWorld = e->GetTransform()->GetLocalToWorld()});
    }
}
