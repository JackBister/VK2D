#pragma once

#pragma once

#include <optional>

#include "Component.h"
#include "Core/Rendering/Particles/ParticleEmitter.h"
#include "Core/Rendering/StaticMeshInstance.h"
#include "Util/DllExport.h"

class ParticleSystem;

class EAPI ParticleEmitterComponent : public Component
{
public:
    ParticleEmitterComponent(ParticleSystem * particleSystem, ParticleEmitterId particleEmitterId,
                             std::string const & imageFilename);
    ~ParticleEmitterComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args) override;

    REFLECT()
    REFLECT_INHERITANCE()

private:
    ParticleSystem * particleSystem;
    ParticleEmitterId particleEmitterId;
    std::string imageFilename;
};
