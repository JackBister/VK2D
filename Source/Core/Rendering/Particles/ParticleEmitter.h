#pragma once

#include <cstdint>

#include <ThirdParty/glm/glm/glm.hpp>

class ImageViewHandle;
class ParticleSystem;

using ParticleEmitterId = size_t;

struct UpdateParticleEmitter {
    ParticleEmitterId id;
    glm::mat4 localToWorld;
};

class ParticleEmitter
{
public:
    friend class ParticleSystem;

    ParticleEmitter(bool isActive, bool isWorldSpace, uint32_t numParticles, bool isEmissionEnabled, float emissionRate,
                    float lifetime, glm::vec3 initialVelocityMin, glm::vec3 initialVelocityMax,
                    std::vector<glm::vec3> accelerators, glm::vec2 initialSizeMin, glm::vec2 initialSizeMax,
                    bool alwaysSquareSize, ImageViewHandle * image)
        : isActive(isActive), isWorldSpace(isWorldSpace), numParticles(numParticles),
          isEmissionEnabled(isEmissionEnabled), emissionRate(emissionRate), lifetime(lifetime),
          initialVelocityMin(initialVelocityMin), initialVelocityMax(initialVelocityMax), accelerators(accelerators),
          initialSizeMin(initialSizeMin), initialSizeMax(initialSizeMax), alwaysSquareSize(alwaysSquareSize),
          image(image)
    {
    }

    bool IsActive() const { return isActive; }
    bool IsWorldSpace() const { return isWorldSpace; }
    uint32_t GetNumParticles() const { return numParticles; }

    bool IsEmissionEnabled() const { return isEmissionEnabled; }
    float GetEmissionRate() const { return emissionRate; }

    float GetLifetime() const { return lifetime; }

    glm::vec3 GetInitialVelocityMin() const { return initialVelocityMin; }
    glm::vec3 GetInitialVelocityMax() const { return initialVelocityMax; }

    std::vector<glm::vec3> GetAccelerators() const { return accelerators; }

    glm::vec2 GetInitialSizeMin() const { return initialSizeMin; }
    glm::vec2 GetInitialSizeMax() const { return initialSizeMax; }
    bool IsAlwaysSquareSize() const { return alwaysSquareSize; }

    ImageViewHandle * const & GetImage() const { return image; }

private:
    bool isActive;
    bool isWorldSpace;
    uint32_t numParticles;

    bool isEmissionEnabled;
    float emissionRate;

    float lifetime;

    glm::vec3 initialVelocityMin;
    glm::vec3 initialVelocityMax;
    std::vector<glm::vec3> accelerators;

    glm::vec2 initialSizeMin;
    glm::vec2 initialSizeMax;
    bool alwaysSquareSize;

    ImageViewHandle * image;

    glm::mat4 localToWorld;
    float emitCounter = 0.f;
    float simulationTime = 0.f;
    uint32_t numActiveParticles = 0;
};
