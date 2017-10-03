#include "Core/transform.h"

#include "json.hpp"
#include "lua/lua.hpp"

#include "rttr/method.h"
#include "rttr/property.h"
#include "rttr/registration.h"
#include "rttr/variant.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Transform>("Transform")
	.constructor<>()
	.method("local_to_parent", &Transform::local_to_parent)
	.method("local_to_world", &Transform::local_to_world)
	.method("get_parent", &Transform::get_parent)
	.method("get_position", &Transform::get_position)
	.method("get_rotation", &Transform::get_rotation)
	.method("get_scale", &Transform::get_scale)
	.method("set_parent", &Transform::set_parent)
	.method("set_position", static_cast<void(Transform::*)(Vec3 const&)>(&Transform::set_position))
	.method("set_position", static_cast<void(Transform::*)(Vec3 *)>(&Transform::set_position))
	.method("set_rotation", &Transform::set_rotation)
	.method("set_scale", &Transform::set_scale);
}

#if 0
static int TypeError(lua_State * L) {
	lua_pushstring(L, "Error while casting to lua value.\n");
	lua_error(L);
	return 1;
}

static int VPushToLua(lua_State * L, rttr::variant const& v)
{
	bool ok = false;
	if (v.get_type().is_arithmetic()) {
		auto val = v.to_double(&ok);
		if (!ok) {
			return TypeError(L);
		}
		lua_pushnumber(L, val);
		return 1;
	}
	if (v.get_type().is_pointer()) {
		auto serializable = v.convert<LuaSerializable<T> *>();
		serializable->PushToLua(L);
		return 1;
	}

	auto mem = lua_newuserdata(L, v.get_type().get_sizeof());
	//God damn it. rttr doesn't expose a way to get the underlying pointer in a non-typesafe way.
	auto p = (Vec3 *) *(void **)&v;
	memcpy(mem, p, v.get_type().get_sizeof());
	if (luaL_getmetatable(L, "_LuaSerializableStackMT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_LuaSerializableStackMT");
		lua_pushcfunction(L, LuaSerializable::StaticStackLuaIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, LuaSerializable::StaticStackLuaNewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
	return 1;
}

template <typename T>
struct LuaMethod {
	rttr::method method;
	T * receiver;

	static int Invoke(lua_State * L)
	{
		if (!lua_isuserdata(L, 1)) {
			printf("[ERROR] LuaMethod::Invoke with non-pointer first argument");
			assert(false);
		}
		LuaMethod<T> * meth = (LuaMethod<T> *) lua_touserdata(L, 1);
		auto top = lua_gettop(L);
		if (top == 1) {
			rttr::variant result = meth->method.invoke(meth->receiver);
			if (!result.is_valid()) {
				lua_pushnil(L);
				return 1;
			}
			return VPushToLua(L, result);
		}
		lua_pushnil(L);
		return 1;
	}
};

int Transform::LuaIndex(lua_State * L) {
	const char * idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	auto tfmType = rttr::type::get<Transform>();
	auto prop = tfmType.get_property(idx);
	if (!prop.is_valid()) {
		auto meth = tfmType.get_method(idx);
		if (!meth.is_valid()) {
			lua_pushnil(L);
			return 1;
		}
		//TODO: push fat object with pointer to instance and metatable where __call returns to an invoker function
		//TODO: Allocation
		auto mem = lua_newuserdata(L, sizeof(LuaMethod<Transform>));
		auto luaMeth = new (mem) LuaMethod<Transform>{
			meth,
				this
		};

		if (luaL_getmetatable(L, meth.get_name().c_str()) == LUA_TNIL) {
			lua_pop(L, 1);
			luaL_newmetatable(L, meth.get_name().c_str());
			lua_pushcfunction(L, LuaMethod<Transform>::Invoke);
			lua_setfield(L, -2, "__call");
		}
		lua_setmetatable(L, -2);

		return 1;
	}
	auto type = prop.get_type();
	auto val = prop.get_value(*this);

	if (strcmp("position_", idx) == 0) {
		position_.PushToLua(L);
	} else if (strcmp("scale_", idx) == 0) {
		scale_.PushToLua(L);
	} else if (strcmp("is_parent_dirty_", idx) == 0) {
		lua_pushboolean(L, is_parent_dirty_);
	} else if (strcmp("is_world_dirty_", idx) == 0) {
		lua_pushboolean(L, is_world_dirty_);
	} else {
		lua_pushnil(L);
	}
	return 1;
}
#endif

glm::mat4 const& Transform::local_to_parent()
{
	glm::mat4 trans = glm::translate(glm::mat4(), position_);
	glm::mat4 rotXYZ = glm::mat4_cast(rotation_);
	glm::mat4 transrot = trans * rotXYZ;
	glm::mat4 scalem = glm::scale(glm::mat4(), scale_);
	to_parent_ = transrot * scalem;
	is_parent_dirty_ = false;
	return to_parent_;
}

glm::mat4 const& Transform::local_to_world()
{
	if (is_world_dirty_) {
		if (parent == nullptr) {
			to_world_ = local_to_parent();
		} else {
			to_world_ = parent->local_to_world() * local_to_parent();
		}
		is_world_dirty_ = false;
	}
	return to_world_;
}

Transform * Transform::get_parent() const
{
	return parent;
}

Vec3 const& Transform::get_position() const
{
	return position_;
}

Quat const& Transform::get_rotation() const
{
	return rotation_;
}

Vec3 const& Transform::get_scale() const
{
	return scale_;
}

void Transform::set_parent(Transform * p)
{
	parent = p;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_position(Vec3 const& p)
{
	position_ = p;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_position(Vec3 * p) {
	position_ = *p;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_rotation(Quat const& r)
{
	rotation_ = r;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_scale(Vec3 const& s)
{
	scale_ = s;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

Transform Transform::Deserialize(std::string const& s)
{
	Transform ret;
	auto j = nlohmann::json::parse(s);
	ret.position_.x = j["position"]["x"];
	ret.position_.y = j["position"]["y"];
	ret.position_.z = j["position"]["z"];

	ret.rotation_.x = j["rotation"]["x"];
	ret.rotation_.y = j["rotation"]["y"];
	ret.rotation_.z = j["rotation"]["z"];
	ret.rotation_.w = j["rotation"]["w"];

	ret.scale_.x = j["scale"]["x"];
	ret.scale_.y = j["scale"]["y"];
	ret.scale_.z = j["scale"]["z"];

	return ret;
}
