#pragma once

#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>

struct DebugPoint {
    glm::vec3 worldSpacePos;
    glm::vec3 color;
    float remainingTime;
};

struct DebugLine {
    glm::vec3 worldSpaceStartPos;
    glm::vec3 worldSpaceEndPos;
    glm::vec3 color;
    float remainingTime;
};

struct DebugDraws {
    std::vector<DebugPoint> points;
    std::vector<DebugLine> lines;
};

class DebugDrawSystem
{
public:
    static DebugDrawSystem * GetInstance();

    void DrawPoint(glm::vec3 worldSpacePos, glm::vec3 color, float persistTimeSeconds);
    void DrawLine(glm::vec3 worldSpaceStartPos, glm::vec3 worldSpaceEndPos, glm::vec3 color, float persistTimeSeconds);

    void Tick();
    DebugDraws GetCurrentDraws();

private:
    DebugDrawSystem();

    DebugDraws currentDraws;
};
