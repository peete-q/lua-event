
#ifndef LUA_WEEK_H
#define LUA_WEEK_H

#include <lua.h>

void luaweek_register(lua_State* L);
int luaweek_ref(lua_State* L);
void luaweek_unref(lua_State* L, int ref);
void luaweek_get(lua_State* L, int ref);

#endif
