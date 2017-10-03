#pragma once
#include <vector>

#include "Core/entity.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Math/mathtypes.h"

//TODO: Cannot use headergenerator because of std::vector shenanigans
class CollisionInfo : public LuaSerializable
{
public:
	Entity * other_;
	bool collision_start_ = true;
	std::vector<Vec3> normals_;
	std::vector<Vec3> points_;
};
