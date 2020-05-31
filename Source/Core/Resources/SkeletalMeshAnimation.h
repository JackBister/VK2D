#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

struct Vec3Key {
    double time;
    glm::vec3 value;
};

struct QuatKey {
    double time;
    glm::quat value;
};

struct NodeAnimation {
    std::string nodeName;
    std::vector<Vec3Key> positionKeys;
    std::vector<QuatKey> rotationKeys;
    std::vector<Vec3Key> scaleKeys;

    // TODO: aiAnimBehavior
};

class SkeletalMeshAnimation
{
public:
    SkeletalMeshAnimation(std::string name, double duration, double ticksPerSecond, std::vector<NodeAnimation> channels)
        : name(name), duration(duration), ticksPerSecond(ticksPerSecond), channels(channels)
    {
    }

    inline std::string GetName() const { return name; }
    inline double GetDuration() const { return duration; }
    inline double GetTicksPerSecond() const { return ticksPerSecond; }
    inline std::vector<NodeAnimation> const & GetChannels() const { return channels; }

private:
    std::string name;
    double duration;
    double ticksPerSecond;

    std::vector<NodeAnimation> channels;
};
