
#include <lauxlib.h>

#include "lua_week.h"

static int dummy_;

static void luaweek_gettable(lua_State* L) {
	lua_pushlightuserdata(L, &dummy_);
	lua_gettable(L, LUA_REGISTRYINDEX);
}

void luaweek_register(lua_State* L) {
	lua_pushlightuserdata(L, &dummy_);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushstring(L, "kv");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	lua_settable(L, LUA_REGISTRYINDEX);
}

int luaweek_ref(lua_State* L) {
	int ref;
	luaweek_gettable(L);
	lua_pushvalue(L, -2);
	ref = luaL_ref(L, -2);
	lua_pop(L, 2);
	return ref;
}

void luaweek_unref(lua_State* L, int ref) {
	luaweek_gettable(L);
	luaL_unref(L, -1, ref);
	lua_pop(L, 1);
}

void luaweek_get(lua_State* L, int ref) {
	luaweek_gettable(L);
	lua_rawgeti(L, -1, ref);
	lua_remove(L, -2);
}


