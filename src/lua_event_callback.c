
#include <assert.h>
#include <lauxlib.h>
#include <string.h>

#include "lua_event_callback.h"
#include "lua_week.h"

#define EVENT_CALLBACK_TYPE "*event.core.eventcallback"

/* Index for coroutine is fd as integer for *nix, as lightuserdata for Win */
static void luaeventcallback_handle(int fd, short what, void* p) {
	lua_EventCallback* cb = p;
	lua_State* L;
	if(!cb->event) {
		/* Callback has been collected... die */
		/* TODO: What should really be done here... */
		return;
	}
	assert(cb->event->running);
	L = cb->event->running;
	lua_rawgeti(L, LUA_REGISTRYINDEX, cb->cb_ref);
	luaweek_get(L, cb->ev_ref);
	lua_pushinteger(L, what);
	lua_call(L, 2, 0);
}

static int luaeventcallback_gc(lua_State* L) {
	lua_EventCallback* cb = luaL_checkudata(L, 1, EVENT_CALLBACK_TYPE);
	if(cb->event) {
		cb->event = NULL;
		event_del(cb->ev);
		luaL_unref(L, LUA_REGISTRYINDEX, cb->cb_ref);
	}
	return 0;
}

static int luaeventcallback_add(lua_State* L) {
	lua_EventCallback* cb = luaL_checkudata(L, 1, EVENT_CALLBACK_TYPE);
	if (lua_isnoneornil(L, 2)) {
		event_add(cb->ev, NULL);
	}
	else {
		double timeout = luaL_checknumber(L, 2);
		struct timeval tv;
		luaevent_gettimeval(timeout, &tv);
		event_add(cb->ev, &tv);
	}
	return 0;
}

static int luaeventcallback_del(lua_State* L) {
	lua_EventCallback* cb = luaL_checkudata(L, 1, EVENT_CALLBACK_TYPE);
	event_del(cb->ev);
	return 0;
}

static int luaeventcallback_setpriority(lua_State* L) {
	lua_EventCallback* cb = luaL_checkudata(L, 1, EVENT_CALLBACK_TYPE);
	int priority = luaL_checkint(L, 2);
	event_priority_set(cb->ev, priority);
	return 0;
}

lua_EventCallback* luaeventcallback_new(lua_State* L, lua_Event *event, int fd, int what, int func) {
	lua_EventCallback* cb;
	luaL_checktype(L, func, LUA_TFUNCTION);
	cb = lua_newuserdata(L, sizeof(lua_EventCallback));
	luaL_getmetatable(L, EVENT_CALLBACK_TYPE);
	lua_setmetatable(L, -2);
	
	lua_pushvalue(L, -1);
	cb->ev_ref = luaweek_ref(L);
	
	lua_pushvalue(L, func);
	cb->cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	cb->event = event;
	cb->ev = event_new(event->base, fd, what, luaeventcallback_handle, cb);
	return cb;
}

static luaL_Reg funcs[] = {
	{ "add", luaeventcallback_add },
	{ "del", luaeventcallback_del },
	{ NULL, NULL }
};

int luaeventcallback_register(lua_State* L) {
	luaL_newmetatable(L, EVENT_CALLBACK_TYPE);
	lua_pushcfunction(L, luaeventcallback_gc);
	lua_setfield(L, -2, "__gc");
	lua_newtable(L);
	luaL_register(L, NULL, funcs);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	return 0;
}
