
#ifndef LUA_BUFFER_EVENT_H
#define LUA_BUFFER_EVENT_H
 
#include "lua_event.h"

typedef struct {
	struct bufferevent* bev;
	lua_Event* event;
} lua_BufferEvent;

int luabufferevent_register(lua_State* L);
lua_BufferEvent* luabufferevent_check(lua_State* L, int idx);

#endif
