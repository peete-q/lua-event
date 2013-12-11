
#ifndef LUA_EVENT_CALLBACK_H
#define LUA_EVENT_CALLBACK_H

#include "lua_event.h"

typedef struct {
	struct event* ev;
	lua_Event* event;
	int ev_ref;
	int cb_ref;
} lua_EventCallback;

int luaeventcallback_register(lua_State* L);
lua_EventCallback* luaeventcallback_new(lua_State* L, lua_Event *event, int fd, int what, int func);

#endif
