#pragma once
#include <memory>
#include <string>
#include <vector>

#include "SDL2/SDL.h"

#include "Core/GameModule.h"
#include "Core/HashedString.h"
#include "Core/Input.h"
#include "Core/Queue.h"
#include "Core/dtime.h"
#include "Core/eventarg.h"

// TODO: Allocate all entities/components from same block for cache

class Entity;

class Scene
{
public:
    static Scene * FromFile(std::string const &);

    void SerializeToFile(std::string const & filename);
    void Unload();

    inline std::string GetFileName() const { return fileName; }

private:
    Scene(std::string const & fileName, std::vector<std::string> dlls, std::vector<Entity *> entities);

    std::string fileName;
    std::vector<std::string> dlls;
    std::vector<Entity *> entities;
};
