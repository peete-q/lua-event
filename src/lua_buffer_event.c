
#include <stdlib.h>
#include <lauxlib.h>

#include "lua_buffer_event.h"
#include "lua_event_buffer.h"
#include "lua_week.h"

#define BUFFER_EVENT_TYPE "*event.core.bufferevent"

/* Locations of READ/WRITE buffers in the fenv */
#define READ_BUFFER_LOCATION 4
#define WRITE_BUFFER_LOCATION 5

/* Obtains an lua_BufferEvent structure from a given index */
static lua_BufferEvent* luabufferevent_get(lua_State* L, int idx) {
	return (lua_BufferEvent*)luaL_checkudata(L, idx, BUFFER_EVENT_TYPE);
}

/* Obtains an lua_BufferEvent structure from a given index
	AND checks that it hadn't been prematurely freed
*/
lua_BufferEvent* luabufferevent_check(lua_State* L, int idx) {
	lua_BufferEvent* ev = (lua_BufferEvent*)luaL_checkudata(L, idx, BUFFER_EVENT_TYPE);
	if(!ev->bev)
		luaL_argerror(L, idx, "Attempt to use closed buffer_event object");
	return ev;
}

static void handle_callback(lua_BufferEvent* ev, short what, int callbackIndex) {
	lua_State* L = ev->event->running;
	luaweek_get(L, ev);
	lua_getfenv(L, -1);
	lua_rawgeti(L, -1, callbackIndex);
	lua_remove(L, -2);
	lua_pushvalue(L, -2);
	lua_remove(L, -3);
	/* func, bufferevent */
	lua_pushinteger(L, what);
	/* What to do w/ errors...? */
	if(!lua_pcall(L, 2, 0, 0))
	{
		/* FIXME: Perhaps luaevent users should be
		 * able to set an error handler? */
		lua_pop(L, 1); /* Pop error message */
	}
}

static void luabufferevent_readcb(struct bufferevent *ev, void *ptr) {
	handle_callback((lua_BufferEvent*)ptr, BEV_EVENT_READING, 1);
}

static void luabufferevent_writecb(struct bufferevent *ev, void *ptr) {
	handle_callback((lua_BufferEvent*)ptr, BEV_EVENT_WRITING, 2);
}

static void luabufferevent_errorcb(struct bufferevent *ev, short what, void *ptr) {
	handle_callback((lua_BufferEvent*)ptr, what, 3);
}

/* LUA: new(fd, read, write, error)
	Pushes a new bufferevent instance on the stack
	Accepts: base, fd, read, write, error cb
	Requires base, fd and error cb
*/
static int luabufferevent_new(lua_State* L) {
	lua_BufferEvent *ev;
	lua_Event* event = luaevent_check(L, 1);
	/* NOTE: Should probably reference the socket as well... */
	int fd = luaevent_getfd(L, 2);
	luaL_checktype(L, 5, LUA_TFUNCTION);
	if(!lua_isnil(L, 3)) luaL_checktype(L, 3, LUA_TFUNCTION);
	if(!lua_isnil(L, 4)) luaL_checktype(L, 4, LUA_TFUNCTION);
	ev = (lua_BufferEvent*)lua_newuserdata(L, sizeof(lua_BufferEvent));
	luaL_getmetatable(L, BUFFER_EVENT_TYPE);
	lua_setmetatable(L, -2);
	ev->bev = bufferevent_new(fd, luabufferevent_readcb, luabufferevent_writecb, luabufferevent_errorcb, ev);
	lua_createtable(L, 5, 0);
	lua_pushvalue(L, 3);
	lua_rawseti(L, -2, 1); // Read
	lua_pushvalue(L, 4);
	lua_rawseti(L, -2, 2); // Write
	lua_pushvalue(L, 5);
	lua_rawseti(L, -2, 3); // Err

	luaeventbuffer_push(L, bufferevent_get_input(ev->bev));
	lua_rawseti(L, -2, READ_BUFFER_LOCATION);
	luaeventbuffer_push(L, bufferevent_get_output(ev->bev));
	lua_rawseti(L, -2, WRITE_BUFFER_LOCATION);
	lua_setfenv(L, -2);
	ev->event = event;
	return 1;
}

/* LUA: __gc and bufferevent:close()
	Releases the bufferevent resources
*/
static int luabufferevent_gc(lua_State* L) {
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(ev->bev) {
		lua_EventBuffer *read, *write;
		bufferevent_free(ev->bev);
		ev->bev = NULL;
		/* Also clear out the associated input/output event_buffers
		 * since they would have already been freed.. */
		lua_getfenv(L, 1);
		lua_rawgeti(L, -1, READ_BUFFER_LOCATION);
		lua_rawgeti(L, -2, WRITE_BUFFER_LOCATION);
		read = luaeventbuffer_check(L, -2);
		write = luaeventbuffer_check(L, -1);
		/* Erase Lua's link to the buffers */
		lua_pushnil(L);
		/* LS: ..., fenv, readBuf, writeBuf, nil */
		lua_rawseti(L, -4, READ_BUFFER_LOCATION);
		lua_pushnil(L);
		lua_rawseti(L, -4, WRITE_BUFFER_LOCATION);
		/* Erase their knowledge of the buffer so that the GC won't try to double-free */
		read->buffer = NULL;
		write->buffer = NULL;
	}
	return 0;
}

static int luabufferevent_get_read(lua_State* L) {
	(void)luabufferevent_get(L, 1);
	lua_getfenv(L, 1);
	lua_rawgeti(L, -1, READ_BUFFER_LOCATION);
	return 1;
}

static int luabufferevent_get_write(lua_State* L) {
	(void)luabufferevent_get(L, 1);
	lua_getfenv(L, 1);
	lua_rawgeti(L, -1, WRITE_BUFFER_LOCATION);
	return 1;
}

static int luabufferevent_set_read_watermarks(lua_State* L) {
	int low, high;
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(!ev->bev) return 0;

	low = lua_tonumber(L, 2);
	high = lua_tonumber(L, 3);

	bufferevent_setwatermark(ev->bev, EV_READ, low, high);
	return 0;
}

static int luabufferevent_set_write_watermarks(lua_State* L) {
	int low, high;
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(!ev->bev) return 0;

	low = lua_tonumber(L, 2);
	high = lua_tonumber(L, 3);

	bufferevent_setwatermark(ev->bev, EV_WRITE, low, high);
	return 0;
}

static int luabufferevent_settimeout(lua_State* L) {
	int timeout_read, timeout_write;
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(!ev->bev) return 0;

	timeout_read = lua_tointeger(L, 2);
	timeout_write = lua_tointeger(L, 3);

	bufferevent_settimeout(ev->bev, timeout_read, timeout_write);
	return 0;
}

static int luabufferevent_enable(lua_State* L) {
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(!ev->bev) return 0;

	lua_pushinteger(L, bufferevent_enable(ev->bev, luaL_checkinteger(L, 2)));
	return 1;
}

static int luabufferevent_disable(lua_State* L) {
	lua_BufferEvent* ev = luabufferevent_get(L, 1);
	if(!ev->bev) return 0;

	lua_pushinteger(L, bufferevent_disable(ev->bev, luaL_checkinteger(L, 2)));
	return 1;
}

static luaL_Reg luabufferevent_funcs[] = {
	{"get_read", luabufferevent_get_read},
	{"get_write", luabufferevent_get_write},
	{"set_read_watermarks", luabufferevent_set_read_watermarks},
	{"set_write_watermarks", luabufferevent_set_write_watermarks},
	{"settimeout", luabufferevent_settimeout},
	{"enable", luabufferevent_enable},
	{"disable", luabufferevent_disable},
	{NULL, NULL}
};

static luaL_Reg funcs[] = {
	{"new", luabufferevent_new},
	{NULL, NULL}
};

int luabufferevent_register(lua_State* L) {
	luaL_newmetatable(L, BUFFER_EVENT_TYPE);
	lua_pushcfunction(L, luabufferevent_gc);
	lua_setfield(L, -2, "__gc");
	lua_newtable(L);
	luaL_register(L, NULL, luabufferevent_funcs);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_register(L, "event.core.bufferevent", funcs);
	return 1;
}
