
#ifndef LUA_EVENT_BUFFER_H
#define LUA_EVENT_BUFFER_H

#include "lua_event.h"

typedef struct {
	struct evbuffer* buffer;
} lua_EventBuffer;

int luaeventbuffer_register(lua_State* L);
int lua_iseventbuffer(lua_State* L, int idx);
lua_EventBuffer* luaeventbuffer_check(lua_State* L, int idx);
int luaeventbuffer_push(lua_State* L, struct evbuffer* buffer);

#endif
