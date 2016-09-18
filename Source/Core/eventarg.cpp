#include "eventarg.h"

#include <cassert>
#include <string>

using namespace std;

//We need to do this both in the move assignment and in destructor
void EventArg::Delete()
{
	if (type == Type::STRING && asString != nullptr) {
		delete asString;
	}
	if (type == Type::EVENTARGS && asEventArgs != nullptr) {
		delete asEventArgs;
	}
	if (type == Type::VECTOR && asVector != nullptr) {
		delete asVector;
	}
}

EventArg::~EventArg()
{
	Delete();
}

EventArg::EventArg()
{
}

EventArg::EventArg(const EventArg& ea) : type(ea.type)
{
	switch (type) {
	case Type::STRING:
		asString = new string(*ea.asString);
		break;
	case Type::INT:
		asInt = ea.asInt;
		break;
	case Type::FLOAT:
		asFloat = ea.asFloat;
		break;
	case Type::DOUBLE:
		asDouble = ea.asDouble;
		break;
	case Type::LUASERIALIZABLE:
		asLuaSerializable = ea.asLuaSerializable;
		break;
	case Type::LUA_CFUNCTION:
		asLuaCFunction = ea.asLuaCFunction;
		break;
	case Type::EVENTARGS:
		asEventArgs = new EventArgs(*ea.asEventArgs);
		break;
	case Type::VECTOR:
		asVector = new std::vector<EventArg>(*ea.asVector);
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
}

EventArg::EventArg(EventArg&& ea) : type(ea.type)
{
	switch (type) {
	case Type::STRING:
		asString = ea.asString;
		ea.asString = nullptr;
		break;
	case Type::INT:
		asInt = ea.asInt;
		break;
	case Type::FLOAT:
		asFloat = ea.asFloat;
		break;
	case Type::DOUBLE:
		asDouble = ea.asDouble;
		break;
	case Type::LUASERIALIZABLE:
		asLuaSerializable = ea.asLuaSerializable;
		break;
	case Type::LUA_CFUNCTION:
		asLuaCFunction = ea.asLuaCFunction;
		break;
	case Type::EVENTARGS:
		asEventArgs = ea.asEventArgs;
		ea.asEventArgs = nullptr;
		break;
	case Type::VECTOR:
		asVector = ea.asVector;
		ea.asVector = nullptr;
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
}

EventArg& EventArg::operator=(const EventArg& ea)
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
		asString = ea.asString;
		ea.asString = nullptr;
		break;
	case Type::INT:
		asInt = ea.asInt;
		break;
	case Type::FLOAT:
		asFloat = ea.asFloat;
		break;
	case Type::DOUBLE:
		asDouble = ea.asDouble;
		break;
	case Type::LUASERIALIZABLE:
		asLuaSerializable = ea.asLuaSerializable;
		break;
	case Type::LUA_CFUNCTION:
		asLuaCFunction = ea.asLuaCFunction;
		break;
	case Type::EVENTARGS:
		asEventArgs = ea.asEventArgs;
		ea.asEventArgs = nullptr;
		break;
	case Type::VECTOR:
		asVector = ea.asVector;
		ea.asVector = nullptr;
		break;
	default:
		printf("[ERROR] Unknown EventArg::type %d\n", type);
	}
	return *this;
}


EventArg::EventArg(string s) : type(Type::STRING), asString(new string(s))
{
}

EventArg::EventArg(const char * s) : type(Type::STRING), asString(new string(s))
{
}
EventArg::EventArg(int i) : type(Type::INT), asInt(i)
{
}

EventArg::EventArg(float f) : type(Type::FLOAT), asFloat(f)
{
}

EventArg::EventArg(double d) : type(Type::DOUBLE), asDouble(d)
{
}

EventArg::EventArg(LuaSerializable * l) : type(Type::LUASERIALIZABLE), asLuaSerializable(l)
{
}

EventArg::EventArg(lua_CFunction cf) : type(Type::LUA_CFUNCTION), asLuaCFunction(cf)
{
}

EventArg::EventArg(EventArgs ea) : type(Type::EVENTARGS), asEventArgs(new EventArgs(ea))
{
}

EventArg::EventArg(const std::vector<EventArg>& v) : type(Type::VECTOR), asVector(new std::vector<EventArg>(v))
{
}

void EventArg::PushToLua(lua_State * L)
{
	switch (type) {
	case Type::STRING:
		lua_pushstring(L, asString->c_str());
		break;
	case Type::INT:
		lua_pushnumber(L, asInt);
		break;
	case Type::FLOAT:
		lua_pushnumber(L, asFloat);
		break;
	case Type::DOUBLE:
		lua_pushnumber(L, asDouble);
		break;
	case Type::LUASERIALIZABLE:
		asLuaSerializable->PushToLua(L);
		break;
	case Type::LUA_CFUNCTION:
		lua_pushcfunction(L, asLuaCFunction);
		break;
	case Type::EVENTARGS:
		lua_createtable(L, 0, static_cast<int>(asEventArgs->size()));
		for (auto& eap : *asEventArgs) {
			lua_pushstring(L, eap.first.c_str());
			eap.second.PushToLua(L);
			lua_settable(L, -3);
		}
		break;
	case Type::VECTOR:
		if (asVector->size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(asVector->size()));
			for (size_t i = 0; i < asVector->size(); ++i) {
				lua_pushnumber(L, i + 1);
				(*asVector)[i].PushToLua(L);
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
		const char * key = lua_tostring(L, -2);
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
