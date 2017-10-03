#include "Core/Lua/luaserializable.h"

#include "rttr/instance.h"
#include "rttr/registration.h"
#include "rttr/variant.h"

#include "Core/eventarg.h"
#include "Core/transform.h"

std::string rttrCharPtrToString(char const * v, bool& ok) {
	ok = true;
	return std::string(v);
}

RTTR_REGISTRATION
{
	rttr::registration::class_<LuaSerializable>("LuaSerializable");
	rttr::type::register_converter_func(rttrCharPtrToString);
}

static int TypeError(lua_State * L) {
	lua_pushstring(L, "Error while casting to lua value.\n");
	lua_error(L);
	return 1;
}

static rttr::variant VPullFromLua(lua_State * L) {
	if (lua_isboolean(L, 3)) {
		return lua_toboolean(L, 3);
	}
	if (lua_iscfunction(L, 3)) {
		return lua_tocfunction(L, 3);
	}
	if (lua_isinteger(L, 3) || lua_isnumber(L, 3)) {
		return lua_tonumber(L, 3);
	}
	if (lua_isstring(L, 3)) {
		return lua_tostring(L, 3);
	}
	if (lua_isuserdata(L, 3)) {
		return lua_touserdata(L, 3);
	}
	return nullptr;
}

static int VPushToLua(lua_State * L, rttr::variant const& v, rttr::type const& t)
{
	bool ok = false;
	if (t.get_name() == "bool") {
		auto val = v.get_value<bool>();
		lua_pushboolean(L, val);
		return 1;
	}
	if (t.is_arithmetic()) {
		auto val = v.to_double(&ok);
		if (!ok) {
			return TypeError(L);
		}
		lua_pushnumber(L, val);
		return 1;
	}
	if (t.is_pointer()) {
		auto serializable = v.convert<LuaSerializable *>();
		if (!serializable) {
			return TypeError(L);
		}
		serializable->PushToLua(L);
		return 1;
	}

	auto mem = lua_newuserdata(L, t.get_sizeof());
	//God damn it. rttr doesn't expose a way to get the underlying pointer in a non-typesafe way.
	auto p = *(void **)&v;
	memcpy(mem, p, t.get_sizeof());
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

static int VPushToLua(lua_State * L, rttr::variant const& v) {
	return VPushToLua(L, v, v.get_type());
}

struct LuaMethod {
	std::vector<rttr::method> method;

	LuaMethod(std::vector<rttr::method> const& method) : method(method) {}

	static int Invoke(lua_State * L)
	{
		if (!lua_isuserdata(L, 1)) {
			printf("[ERROR] LuaMethod::Invoke with non-pointer first argument");
			assert(false);
		}
		auto * meth = (LuaMethod *)lua_touserdata(L, 1);
		auto top = lua_gettop(L);
		if (top < 2) {
			lua_pushstring(L, "__call: attempt to invoke a method with too few arguments.");
			lua_error(L);
			return 1;
		}
		auto receiver = *(LuaSerializable **)lua_touserdata(L, 2);
		if (top == 2) {
			rttr::variant result = meth->method[0].invoke(receiver);
			if (!result.is_valid()) {
				lua_pushstring(L, "__call: invalid result.");
				lua_error(L);
				return 1;
			}
			return VPushToLua(L, result);
		}
		std::vector<rttr::variant> values;
		for (int i = 3; i < top + 1; ++i) {
			auto type = lua_type(L, i);
			rttr::variant value = [L, i, type]() -> rttr::variant {
				switch (type) {
				case LUA_TNIL:
					return nullptr;
				case LUA_TBOOLEAN:
					return lua_toboolean(L, i);
				case LUA_TNUMBER:
					return lua_tonumber(L, i);
				case LUA_TSTRING:
					return lua_tostring(L, i);
				case LUA_TTABLE:
					return PullEventArgs(L, i);
				case LUA_TFUNCTION:
					printf("[STUB] LUA_TFUNCTION\n");
					return nullptr;
				case LUA_TUSERDATA:
				case LUA_TLIGHTUSERDATA:
					return (LuaSerializable *)lua_touserdata(L, i);
				case LUA_TTHREAD:
					printf("[STUB] LUA_TTHREAD\n");
					return nullptr;
				}
				printf("[WARN] %s:%d default\n", __FILE__, __LINE__);
				return nullptr;
			}();
			if (value.is_valid()) {
				values.push_back(value);
			}
		}
		for (auto const& m : meth->method) {
			auto param_info = m.get_parameter_infos();
			if (param_info.size() != values.size()) {
				continue;
			}
			auto is_valid = true;
			for (size_t i = 0; i < values.size(); ++i) {
				if (!values[i].can_convert(param_info[i].get_type())) {
					is_valid = false;
					break;
				}
			}
			if (!is_valid) {
				continue;
			}
			std::vector<rttr::argument> args;
			for (size_t i = 0; i < values.size(); ++i) {
				values[i].convert(param_info[i].get_type());
				args.push_back(values[i]);
			}
			auto result = m.invoke_variadic(receiver, args);
			if (!result.is_valid()) {
				lua_pushstring(L, "__call: invalid result.");
				lua_error(L);
				return 1;
			}
			return VPushToLua(L, result);
		}
	
		lua_pushstring(L, "__call: unable to find method matching given arguments.");
		lua_error(L);
		return 1;
	}
};

int LuaSerializable::LuaIndex(lua_State * L)
{
	const char * idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushstring(L, "__index: indexer must be string.");
		lua_error(L);
		return 1;
	}
	auto tfmType = rttr::type::get(*this);
	auto prop = tfmType.get_property(idx);
	if (!prop.is_valid()) {
		auto meths = tfmType.get_methods();
		std::remove_if(meths.begin(), meths.end(), [idx](rttr::method m) {
			return strcmp(m.get_name().c_str(), idx);
		});
		if (meths.size() == 0) {
			//TODO: Could return an error here, but nil seems more in line with Lua/JS semantics?
			lua_pushnil(L);
			return 1;
		}
		auto mem = lua_newuserdata(L, sizeof(LuaMethod));
		auto luaMeth = new (mem) LuaMethod(meths);

		if (luaL_getmetatable(L, "__LuaMethodInvokeMT") == LUA_TNIL) {
			lua_pop(L, 1);
			luaL_newmetatable(L, "__LuaMethodInvokeMT");
			lua_pushcfunction(L, LuaMethod::Invoke);
			lua_setfield(L, -2, "__call");
		}
		lua_setmetatable(L, -2);

		return 1;
	}

	auto val = prop.get_value(this);
	if (!val.is_valid()) {
		lua_pushstring(L, "__index: property value is invalid.");
		lua_error(L);
		return 1;
	}

	return VPushToLua(L, val, prop.get_type());
}

int LuaSerializable::LuaNewIndex(lua_State * L)
{
	const char * idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushstring(L, "__newindex: indexer must be string.");
		lua_error(L);
		return 0;
	}
	auto tfmType = rttr::type::get(*this);
	auto prop = tfmType.get_property(idx);
	if (!prop.is_valid()) {
		lua_pushstring(L, "__newindex: property not found.");
		lua_error(L);
		return 0;
	}

	auto val = VPullFromLua(L);
	if (!val.is_valid()) {
		lua_pushstring(L, "__newindex: unable to pull value from Lua.");
		lua_error(L);
		return 0;
	}
	if (!val.can_convert(prop.get_type())) {
		lua_pushstring(L, "__newindex: type of property is incompatible with given value.");
		lua_error(L);
		return 0;
	}
	val.convert(prop.get_type());
	auto success = prop.set_value(*this, val);
	if (!success) {
		lua_pushstring(L, "__newindex: unable to set property value.");
		lua_error(L);
	}
	return 0;
}

void LuaSerializable::PushToLua(void * Lx)
{
	auto L = (lua_State *)Lx;
	void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
	*vpp = this;
	if (luaL_getmetatable(L, "_LuaSerializableMT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_LuaSerializableMT");
		lua_pushcfunction(L, StaticLuaIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, StaticLuaNewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
}

int LuaSerializable::StaticLuaIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaIndex(L);
}
int LuaSerializable::StaticLuaNewIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaNewIndex(L);
}

int LuaSerializable::StaticStackLuaIndex(lua_State * L)
{
	auto lsp = (LuaSerializable *)lua_touserdata(L, 1);
	return lsp->LuaIndex(L);
}
int LuaSerializable::StaticStackLuaNewIndex(lua_State * L)
{
	auto lsp = (LuaSerializable *)lua_touserdata(L, 1);
	return lsp->LuaNewIndex(L);
}


#if 0
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

template<typename T>
int LuaSerializable<T>::LuaIndex(lua_State * L)
{
	const char * idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	auto tfmType = rttr::type::get<T>();
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
		auto luaMeth = new (mem) LuaMethod<T>{
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
}

template<typename T>
void LuaSerializable<T>::PushToLua(void * Lx)
{
	auto L = (lua_State *)Lx;
	void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
	*vpp = this;
	if (luaL_getmetatable(L, "_LuaSerializableMT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_LuaSerializableMT");
		lua_pushcfunction(L, StaticLuaIndex);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, StaticLuaNewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
}

template<typename T>
int LuaSerializable<T>::StaticLuaIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaIndex(L);
}

template<typename T>
int LuaSerializable<T>::StaticLuaNewIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaNewIndex(L);
}

template<typename T>
int LuaSerializable<T>::StaticStackLuaIndex(lua_State * L)
{
	//TODO hack because instance lua indexes expect **
	auto lsp = (LuaSerializable *)lua_touserdata(L, 1);
	//auto topPtr = (LuaSerializable **) lua_newuserdata(L, sizeof(LuaSerializable *));
	//*topPtr = lsp;
	return lsp->LuaIndex(L);
}

template<typename T>
int LuaSerializable<T>::StaticStackLuaNewIndex(lua_State * L)
{
	//TODO hack because instance lua indexes expect **
	auto lsp = (LuaSerializable *)lua_touserdata(L, 1);
//	auto topPtr = (LuaSerializable **)lua_newuserdata(L, sizeof(LuaSerializable *));
//	*topPtr = lsp;
	return lsp->LuaNewIndex(L);
}

template<typename T>
LuaSerializable<T>::~LuaSerializable() {}
#endif
