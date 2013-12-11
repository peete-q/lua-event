
#include <lauxlib.h>
#include <assert.h>
#include <string.h>

#include "lua_event_buffer.h"
#include "lua_buffer_event.h"
#include "lua_week.h"

#define EVENT_BASE_TYPE "*event.core.base"

lua_Event* luaevent_check(lua_State* L, int idx) {
	return (lua_Event*)luaL_checkudata(L, idx, EVENT_BASE_TYPE);
}

int luaevent_newbase(lua_State* L) {
	lua_Event *event = (lua_Event*)lua_newuserdata(L, sizeof(lua_Event));
	event->running = NULL; /* No running loop */
	event->base = event_init();
	luaL_getmetatable(L, EVENT_BASE_TYPE);
	lua_setmetatable(L, -2);
	return 1;
}

static int luaevent_getlibeventversion(lua_State* L) {
	lua_pushstring(L, event_get_version());
	return 1;
}

static int luaevent_getsupportedmethods(lua_State* L) {
    const char **methods = event_get_supported_methods();
    int n = 0;
    while (methods[n])
        lua_pushstring(L, methods[n++]);
    return n;
}

static int luaeventbase_gc(lua_State* L) {
	lua_Event *event = luaevent_check(L, 1);
	if(event->base) {
		event_base_free(event->base);
		event->base = NULL;
	}
	return 0;
}

int luaevent_getfd(lua_State* L, int idx) {
	int fd;
	if(lua_isnumber(L, idx)) {
		fd = lua_tonumber(L, idx);
	} else {
		luaL_checktype(L, idx, LUA_TUSERDATA);
		lua_getfield(L, idx, "getfd");
		if(lua_isnil(L, -1))
			return luaL_error(L, "Socket type missing 'getfd' method");
		lua_pushvalue(L, idx);
		lua_call(L, 1, 1);
		fd = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	return fd;
}

void luaevent_gettimeval(double time, struct timeval *tv) {
	tv->tv_sec = (int) time;
	tv->tv_usec = (int)( (time - tv->tv_sec) * 1000000 );
}

static int luaeventbase_newevent(lua_State* L) {
	int fd, what;
	lua_Event* event = luaevent_check(L, 1);
	if(lua_isnil(L, 2)) {
		fd = -1; /* Per event_timer_set.... */
	} else {
		fd = luaevent_getfd(L, 2);
	}
	what = luaL_checkinteger(L, 3);
	luaeventcallback_new(L, event, fd, what, 4);
	return 1;
}

static int luaeventbase_loop(lua_State* L) {
	int ret;
	lua_Event *event = luaevent_check(L, 1);
	int flags = luaL_optint(L, 2, 0);
	event->running = L;
	ret = event_base_loop(event->base, flags);
	lua_pushinteger(L, ret);
	return 1;
}

static int luaeventbase_loopexit(lua_State*L) {
	int ret;
	lua_Event *event = luaevent_check(L, 1);
	if (lua_isnoneornil(L, 2)) {
		ret = event_base_loopexit(event->base, NULL);
	}
	else {
		double timeout = luaL_checknumber(L, 2);
		struct timeval tv;
		luaevent_gettimeval(timeout, &tv);
		ret = event_base_loopexit(event->base, &tv);
	}
	lua_pushinteger(L, ret);
	return 1;
}

static int luaeventbase_loopbreak(lua_State*L) {
	int ret;
	lua_Event *event = luaevent_check(L, 1);
	ret = event_base_loopbreak(event->base);
	lua_pushinteger(L, ret);
	return 1;
}

static int luaeventbase_getmethod(lua_State* L) {
	lua_Event *event = luaevent_check(L, 1);
	lua_pushstring(L, event_base_get_method(event->base));
	return 1;
}

static luaL_Reg base_funcs[] = {
	{ "newevent", luaeventbase_newevent },
	{ "loop", luaeventbase_loop },
	{ "loopexit", luaeventbase_loopexit },
	{ "loopbreak", luaeventbase_loopbreak },
	{ "getmethod", luaeventbase_getmethod },
	{ NULL, NULL }
};

static luaL_Reg core_funcs[] = {
	{ "newbase", luaevent_newbase },
	{ "getlibeventversion", luaevent_getlibeventversion },
	{ "getsupportedmethods", luaevent_getsupportedmethods},
	{ NULL, NULL }
};

typedef struct {
	const char* name;
	int value;
} int_entry;

static int_entry consts[] = {
	/* feature */
	{"EV_FEATURE_ET", EV_FEATURE_ET},
	{"EV_FEATURE_O1", EV_FEATURE_O1},
	{"EV_FEATURE_FDS", EV_FEATURE_FDS},
	/* base */
	{"EVENT_BASE_FLAG_NOLOCK", EVENT_BASE_FLAG_NOLOCK},
	{"EVENT_BASE_FLAG_IGNORE_ENV", EVENT_BASE_FLAG_IGNORE_ENV},
	{"EVENT_BASE_FLAG_STARTUP_IOCP", EVENT_BASE_FLAG_STARTUP_IOCP},
	{"EVENT_BASE_FLAG_NO_CACHE_TIME", EVENT_BASE_FLAG_NO_CACHE_TIME},
	{"EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST", EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST},
	/* event */
	{"EV_READ", EV_READ},
	{"EV_WRITE", EV_WRITE},
	{"EV_TIMEOUT", EV_TIMEOUT},
	{"EV_SIGNAL", EV_SIGNAL},
	{"EV_PERSIST", EV_PERSIST},
	{"EV_ET", EV_ET},
	/* evloop */
	{"EVLOOP_ONCE", EVLOOP_ONCE},
	{"EVLOOP_NONBLOCK", EVLOOP_NONBLOCK},
	/* bufferevent filter result*/
	{"BEV_OK", BEV_OK},
	{"BEV_NEED_MORE", BEV_NEED_MORE},
	{"BEV_ERROR", BEV_ERROR},
	/* bufferevent flush mode*/
	{"BEV_NORMAL", BEV_NORMAL},
	{"BEV_FLUSH", BEV_FLUSH},
	{"BEV_FINISHED", BEV_FINISHED},
	/* bufferevent options*/
	{"BEV_OPT_CLOSE_ON_FREE ", BEV_OPT_CLOSE_ON_FREE },
	{"BEV_OPT_THREADSAFE", BEV_OPT_THREADSAFE},
	{"BEV_OPT_DEFER_CALLBACKS", BEV_OPT_DEFER_CALLBACKS},
	{"BEV_OPT_UNLOCK_CALLBACKS", BEV_OPT_UNLOCK_CALLBACKS},
	/* bufferevent */
	{"BEV_EVENT_READING", BEV_EVENT_READING},
	{"BEV_EVENT_WRITING", BEV_EVENT_WRITING},
	{"BEV_EVENT_EOF", BEV_EVENT_EOF},
	{"BEV_EVENT_ERROR", BEV_EVENT_ERROR},
	{"BEV_EVENT_TIMEOUT", BEV_EVENT_TIMEOUT},
	{NULL, 0}
};

void lua_expints(lua_State* L, int_entry* p) {
	while(p->name) {
		lua_pushinteger(L, p->value);
		lua_setfield(L, -2, p->name);
		p++;
	}
}

/* Verified ok */
int luaopen_event_core(lua_State* L) {
#ifdef _WIN32
	WORD version = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(version, &wsaData);
#endif
	event_init();
	/* Register external items */
	luaeventcallback_register(L);
	luaeventbuffer_register(L);
	luabufferevent_register(L);
	luaweek_register(L);
	lua_settop(L, 0);
	/* Setup metatable */
	luaL_newmetatable(L, EVENT_BASE_TYPE);
	lua_newtable(L);
	luaL_register(L, NULL, base_funcs);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, luaeventbase_gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);

	luaL_register(L, "event.core", core_funcs);
	lua_expints(L, consts);
	return 1;
}

