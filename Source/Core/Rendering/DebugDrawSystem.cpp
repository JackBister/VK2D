#include "DebugDrawSystem.h"

#include <algorithm>

#include "Core/Console/Console.h"
#include "Core/dtime.h"

DebugDrawSystem * DebugDrawSystem::GetInstance()
{
    static DebugDrawSystem instance;
    return &instance;
}

void DebugDrawSystem::DrawPoint(glm::vec3 worldSpacePos, glm::vec3 color, float persistTimeSeconds)
{
    currentDraws.points.push_back({worldSpacePos, color, persistTimeSeconds});
}

void DebugDrawSystem::DrawLine(glm::vec3 worldSpaceStartPos, glm::vec3 worldSpaceEndPos, glm::vec3 color,
                               float persistTimeSeconds)
{
    currentDraws.lines.push_back({worldSpaceStartPos, worldSpaceEndPos, color, persistTimeSeconds});
}

void DebugDrawSystem::Tick()
{
    auto elapsed = Time::GetUnscaledDeltaTime();

    for (auto & point : currentDraws.points) {
        point.remainingTime -= elapsed;
    }

    for (auto & line : currentDraws.lines) {
        line.remainingTime -= elapsed;
    }

    if (currentDraws.points.size() > 0) {
        currentDraws.points.erase(std::remove_if(currentDraws.points.begin(),
                                                 currentDraws.points.end(),
                                                 [](auto const & p) { return p.remainingTime <= 0.f; }),
                                  currentDraws.points.end());
    }
    if (currentDraws.lines.size() > 0) {
        currentDraws.lines.erase(std::remove_if(currentDraws.lines.begin(),
                                                currentDraws.lines.end(),
                                                [](auto const & l) { return l.remainingTime <= 0.f; }),
                                 currentDraws.lines.end());
    }
}

DebugDraws DebugDrawSystem::GetCurrentDraws()
{
    return currentDraws;
}

DebugDrawSystem::DebugDrawSystem()
{
    Console::RegisterCommand(
        CommandDefinition("debug_draw_line",
                          "debug_draw_line x y z x2 y2 z2 - Draw a line from (x, y, z) to (x2, y2, z2) in world space",
                          6,
                          [](auto args) {
                              auto x = atof(args[0].c_str());
                              auto y = atof(args[1].c_str());
                              auto z = atof(args[2].c_str());
                              auto x2 = atof(args[3].c_str());
                              auto y2 = atof(args[4].c_str());
                              auto z2 = atof(args[5].c_str());

                              DebugDrawSystem::GetInstance()->DrawLine(
                                  glm::vec3(x, y, z), glm::vec3(x2, y2, z2), glm::vec3(1.f, 1.f, 1.f), 5.f);
                          }));

    Console::RegisterCommand(CommandDefinition(
        "debug_draw_point", "debug_draw_point x y z - Draw a point at (x, y, z) in world space", 3, [](auto args) {
            auto x = atof(args[0].c_str());
            auto y = atof(args[1].c_str());
            auto z = atof(args[2].c_str());

            DebugDrawSystem::GetInstance()->DrawPoint(glm::vec3(x, y, z), glm::vec3(1.f, 1.f, 1.f), 5.f);
        }));
}
