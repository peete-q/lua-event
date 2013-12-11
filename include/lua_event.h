
#ifndef LUAEVENT_H
#define LUAEVENT_H

#include <lua.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif
#include <event2/event.h>
#include <event2/bufferevent.h>

typedef struct {
	struct event_base* base;
	lua_State* running;
} lua_Event;

lua_Event* luaevent_check(lua_State* L, int idx);
void luaevent_gettimeval(double time, struct timeval *tv);
int luaevent_getfd(lua_State* L, int idx);

int luaopen_event_core(lua_State* L);

#endif
