#include "Core/eventarg.h"

#include <string>

#include "Core/Lua/LuaSerializable.h"

//We need to do this both in the move assignment and in destructor
void EventArg::Delete()
{
	if (type == Type::STRING && as_string_ != nullptr) {
		delete as_string_;
	}
	if (type == Type::EVENTARGS && as_EventArgs_ != nullptr) {
		delete as_EventArgs_;
	}
	if (type == Type::VECTOR && as_vector_ != nullptr) {
		delete as_vector_;
	}
}

EventArg::~EventArg()
{
	Delete();
}

EventArg::EventArg()
{
}

EventArg::EventArg(EventArg const& ea) : type(ea.type)
{
	switch (type) {
	case Type::STRING:
		as_string_ = new std::string(*ea.as_string_);
		break;
	case Type::INT:
		as_int_ = ea.as_int_;
		break;
	case Type::FLOAT:
		as_float_ = ea.as_float_;
		break;
	case Type::DOUBLE:
		as_double_ = ea.as_double_;
		break;
	case Type::LUASERIALIZABLE:
		as_LuaSerializable_ = ea.as_LuaSerializable_;
		break;
	case Type::LUA_CFUNCTION:
		as_lua_cfunction_ = ea.as_lua_cfunction_;
		break;
	case Type::EVENTARGS:
		as_EventArgs_ = new EventArgs(*ea.as_EventArgs_);
		break;
	case Type::VECTOR:
		as_vector_ = new std::vector<EventArg>(*ea.as_vector_);
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
}

EventArg::EventArg(EventArg&& ea) : type(ea.type)
{
	switch (type) {
	case Type::STRING:
		as_string_ = ea.as_string_;
		ea.as_string_ = nullptr;
		break;
	case Type::INT:
		as_int_ = ea.as_int_;
		break;
	case Type::FLOAT:
		as_float_ = ea.as_float_;
		break;
	case Type::DOUBLE:
		as_double_ = ea.as_double_;
		break;
	case Type::LUASERIALIZABLE:
		as_LuaSerializable_ = ea.as_LuaSerializable_;
		break;
	case Type::LUA_CFUNCTION:
		as_lua_cfunction_ = ea.as_lua_cfunction_;
		break;
	case Type::EVENTARGS:
		as_EventArgs_ = ea.as_EventArgs_;
		ea.as_EventArgs_ = nullptr;
		break;
	case Type::VECTOR:
		as_vector_ = ea.as_vector_;
		ea.as_vector_ = nullptr;
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
}

EventArg& EventArg::operator=(EventArg const& ea)
{
	EventArg tmp(ea);
	*this = std::move(tmp);
	return *this;
}

EventArg& EventArg::operator=(EventArg&& ea)
{
	Delete();
	type = ea.type;
	switch (type) {
	case Type::STRING:
		as_string_ = ea.as_string_;
		ea.as_string_ = nullptr;
		break;
	case Type::INT:
		as_int_ = ea.as_int_;
		break;
	case Type::FLOAT:
		as_float_ = ea.as_float_;
		break;
	case Type::DOUBLE:
		as_double_ = ea.as_double_;
		break;
	case Type::LUASERIALIZABLE:
		as_LuaSerializable_ = ea.as_LuaSerializable_;
		break;
	case Type::LUA_CFUNCTION:
		as_lua_cfunction_ = ea.as_lua_cfunction_;
		break;
	case Type::EVENTARGS:
		as_EventArgs_ = ea.as_EventArgs_;
		ea.as_EventArgs_ = nullptr;
		break;
	case Type::VECTOR:
		as_vector_ = ea.as_vector_;
		ea.as_vector_ = nullptr;
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
	return *this;
}

EventArg::EventArg(std::string s) : type(Type::STRING), as_string_(new std::string(s))
{
}

EventArg::EventArg(char const * s) : type(Type::STRING), as_string_(new std::string(s))
{
}
EventArg::EventArg(int i) : type(Type::INT), as_int_(i)
{
}

EventArg::EventArg(float f) : type(Type::FLOAT), as_float_(f)
{
}

EventArg::EventArg(double d) : type(Type::DOUBLE), as_double_(d)
{
}

EventArg::EventArg(LuaSerializable * l) : type(Type::LUASERIALIZABLE), as_LuaSerializable_(l)
{
}

EventArg::EventArg(lua_CFunction cf) : type(Type::LUA_CFUNCTION), as_lua_cfunction_(cf)
{
}

EventArg::EventArg(EventArgs ea) : type(Type::EVENTARGS), as_EventArgs_(new EventArgs(ea))
{
}

EventArg::EventArg(std::vector<EventArg> const& v) : type(Type::VECTOR), as_vector_(new std::vector<EventArg>(v))
{
}

void EventArg::PushToLua(lua_State * L)
{
	switch (type) {
	case Type::STRING:
		lua_pushstring(L, as_string_->c_str());
		break;
	case Type::INT:
		lua_pushnumber(L, as_int_);
		break;
	case Type::FLOAT:
		lua_pushnumber(L, as_float_);
		break;
	case Type::DOUBLE:
		lua_pushnumber(L, as_double_);
		break;
	case Type::LUASERIALIZABLE:
		as_LuaSerializable_->PushToLua(L);
		break;
	case Type::LUA_CFUNCTION:
		lua_pushcfunction(L, as_lua_cfunction_);
		break;
	case Type::EVENTARGS:
		lua_createtable(L, 0, static_cast<int>(as_EventArgs_->size()));
		for (auto& eap : *as_EventArgs_) {
			lua_pushstring(L, eap.first.c_str());
			eap.second.PushToLua(L);
			lua_settable(L, -3);
		}
		break;
	case Type::VECTOR:
		if (as_vector_->size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(as_vector_->size()));
			for (size_t i = 0; i < as_vector_->size(); ++i) {
				lua_pushnumber(L, static_cast<lua_Number>(i + 1));
				(*as_vector_)[i].PushToLua(L);
				lua_settable(L, -3);
			}
		}
		break;
	default:
		printf("[ERROR] Unknown arg type in EventArg::PushToLua: %d\n", type);
		lua_pushnil(L);
	}
}

EventArgs PullEventArgs(lua_State * L, int i)
{
	EventArgs eargs;
	lua_pushnil(L);
	while (lua_next(L, i) != 0) {
		if (lua_type(L, -2) != LUA_TSTRING) {
			continue;
		}
		char const * key = lua_tostring(L, -2);
		switch (lua_type(L, -1)) {
		case LUA_TNUMBER:
			eargs[key] = lua_tonumber(L, -1);
			break;
		case LUA_TBOOLEAN:
			eargs[key] = lua_toboolean(L, -1);
			break;
		case LUA_TSTRING:
			eargs[key] = lua_tostring(L, -1);
			break;
		case LUA_TTABLE:
			//TODO:
			printf("[STUB] PullEventArgs case LUA_TTABLE\n");
			break;
		case LUA_TFUNCTION:
			//TODO
			printf("[STUB] PullEventArgs case LUA_TCFUNCTION\n");
			break;
		case LUA_TUSERDATA:
		{
			auto ptr = static_cast<LuaSerializable **>(lua_touserdata(L, -1));
			eargs[key] = *ptr;
			break;
		}
		case LUA_TTHREAD:
			printf("[STUB] PullEventArgs case LUA_TTHREAD\n");
			break;
		case LUA_TLIGHTUSERDATA:
			printf("[STUB] PullEventArgs case LUA_TLIGHTUSERDATA\n");
			break;
		default:
			printf("[WARNING] PullEventArgs default case\n");
		}
		lua_pop(L, 1);
	}
	return eargs;
}
