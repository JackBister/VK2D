#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "rttr/rttr_enable.h"

#include "Core/Lua/luaserializable.h"

struct Vec3 : glm::vec3, LuaSerializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	using glm::vec3::tvec3;

	//TODO: Neccessary for now because rttr can't figure out the raw properties without explicitly upcasting
	inline float GetX() const {
		return x;
	}

	inline void SetX(float x) {
		this->x = x;
	}

	inline float GetY() const {
		return y;
	}

	inline void SetY(float y) {
		this->y = y;
	}

	inline float GetZ() const {
		return z;
	}

	inline void SetZ(float z) {
		this->z = z;
	}
};

struct Quat : glm::quat, LuaSerializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	using glm::quat::tquat;

	inline float GetX() const {
		return x;
	}

	inline void SetX(float x) {
		this->x = x;
	}

	inline float GetY() const {
		return y;
	}

	inline void SetY(float y) {
		this->y = y;
	}

	inline float GetZ() const {
		return z;
	}

	inline void SetZ(float z) {
		this->z = z;
	}

	inline float GetW() const {
		return w;
	}

	inline void SetW(float w) {
		this->w = w;
	}
};
