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
	inline float get_x() const {
		return x;
	}

	inline void set_x(float x) {
		this->x = x;
	}

	inline float get_y() const {
		return y;
	}

	inline void set_y(float y) {
		this->y = y;
	}

	inline float get_z() const {
		return z;
	}

	inline void set_z(float z) {
		this->z = z;
	}
};

struct Quat : glm::quat, LuaSerializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	using glm::quat::tquat;

	inline float get_x() const {
		return x;
	}

	inline void set_x(float x) {
		this->x = x;
	}

	inline float get_y() const {
		return y;
	}

	inline void set_y(float y) {
		this->y = y;
	}

	inline float get_z() const {
		return z;
	}

	inline void set_z(float z) {
		this->z = z;
	}

	inline float get_w() const {
		return w;
	}

	inline void set_w(float w) {
		this->w = w;
	}
};
