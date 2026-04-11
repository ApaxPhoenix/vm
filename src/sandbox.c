#include "sandbox.h"
#include "bridge.h"
#include <stdlib.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luajit.h>
#include <assert.h>

#if defined(_WIN32)
#include <windows.h>
static void restrict_resources(void) {
    static int once = 0;
    if (once) return;
    once = 1;
    HANDLE job = CreateJobObject(NULL, NULL);
    assert(job != NULL);
    if (!job) return;
    JOBOBJECT_BASIC_LIMIT_INFORMATION limits = {0};
    limits.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    limits.ActiveProcessLimit = 1;
    assert(SetInformationJobObject(job, JobObjectBasicLimitInformation, &limits, sizeof(limits)));
    assert(AssignProcessToJobObject(job, GetCurrentProcess()));
}
#elif defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
#include <sys/resource.h>
static void restrict_resources(void) {
    static int once = 0;
    if (once) return;
    once = 1;
    struct rlimit limit;
    limit = (struct rlimit){8, 8}; setrlimit(RLIMIT_NOFILE, &limit);
    limit = (struct rlimit){0, 0}; setrlimit(RLIMIT_NPROC, &limit);
    limit = (struct rlimit){0, 0}; setrlimit(RLIMIT_CORE, &limit);
    limit = (struct rlimit){8<<20, 8<<20}; setrlimit(RLIMIT_STACK, &limit);
}
#else
static void restrict_resources(void) {}
#endif

static void governor(lua_State *L, lua_Debug *debug) {
    (void)debug;
    lua_sethook(L, governor, LUA_MASKCOUNT, 1);
    luaL_error(L, "sandbox: cpu budget exceeded");
}

static int loadfile_hook(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, path);
    if (!lua_isnil(L, -1)) return 1;
    lua_pop(L, 1);
    if (luaL_loadfile(L, path) != LUA_OK) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
    return 1;
}

static int coroutine_create(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "coroutine");
    int count = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (count >= MAX_COROUTINES)
        return luaL_error(L, "sandbox: coroutine limit reached (%d)", MAX_COROUTINES);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_State *thread = lua_newthread(L);
    lua_sethook(thread, governor, LUA_MASKCOUNT, SANDBOX_INSTRUCTIONS);
    lua_pushvalue(L, 1);
    lua_xmove(L, thread, 1);
    lua_pushinteger(L, count + 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "coroutine");
    return 1;
}

static int coroutine_resume(lua_State *L) {
    lua_State *thread = lua_tothread(L, 1);
    if (!thread) return luaL_error(L, "sandbox: expected coroutine");
    int arguments = lua_gettop(L) - 1;
    lua_xmove(L, thread, arguments);
    int result = lua_resume(thread, arguments);
    if (result == LUA_OK || result == LUA_YIELD) {
        int returned = lua_gettop(thread);
        lua_xmove(thread, L, returned);
        if (result == LUA_OK) {
            lua_getfield(L, LUA_REGISTRYINDEX, "coroutine");
            int count = lua_tointeger(L, -1);
            lua_pop(L, 1);
            if (count > 0) { lua_pushinteger(L, count - 1); lua_setfield(L, LUA_REGISTRYINDEX, "coroutine"); }
        }
        return lua_gettop(L) - 1;
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "coroutine");
    int count = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (count > 0) { lua_pushinteger(L, count - 1); lua_setfield(L, LUA_REGISTRYINDEX, "coroutine"); }
    lua_pushnil(L);
    lua_pushstring(L, lua_tostring(thread, -1));
    return 2;
}

Sandbox *sandbox_create(Context context) {
    restrict_resources();

    Sandbox *sandbox = malloc(sizeof(Sandbox));
    assert(sandbox != NULL);
    if (!sandbox) return NULL;

    sandbox->context = context;
    sandbox->allocator = allocator_create(SANDBOX_MEMORY);
    assert(sandbox->allocator != NULL);
    if (!sandbox->allocator) { free(sandbox); return NULL; }

    sandbox->state = lua_newstate(allocator_func, sandbox->allocator);
    assert(sandbox->state != NULL);
    if (!sandbox->state) { allocator_destroy(sandbox->allocator); free(sandbox); return NULL; }

    lua_State *L = sandbox->state;

    lua_pushcfunction(L, luaopen_base); lua_pushstring(L, ""); lua_call(L, 1, 0);
    lua_pushcfunction(L, luaopen_table); lua_pushstring(L, LUA_TABLIBNAME); lua_call(L, 1, 0);
    lua_pushcfunction(L, luaopen_string); lua_pushstring(L, LUA_STRLIBNAME); lua_call(L, 1, 0);
    lua_pushcfunction(L, luaopen_math); lua_pushstring(L, LUA_MATHLIBNAME); lua_call(L, 1, 0);
    lua_pushcfunction(L, luaopen_bit); lua_pushstring(L, LUA_BITLIBNAME); lua_call(L, 1, 0);

    lua_pushstring(L, VM_PATH); lua_setglobal(L, "VM_PATH");
    lua_pushcfunction(L, loadfile_hook); lua_setglobal(L, "loadfile");

    bridge_create(L);

    if (context == SANDBOX_SERVER) {
        lua_pushcfunction(L, luaopen_io); lua_pushstring(L, LUA_IOLIBNAME); lua_call(L, 1, 0);
        lua_pushcfunction(L, luaopen_os); lua_pushstring(L, LUA_OSLIBNAME); lua_call(L, 1, 0);
        lua_pushcfunction(L, luaopen_package); lua_pushstring(L, LUA_LOADLIBNAME); lua_call(L, 1, 0);

        if (luaL_loadfile(L, SANDBOX_PATH("signal.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/signal.lua"); else lua_pop(L, 1);
        if (luaL_loadfile(L, SANDBOX_PATH("storage/server.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/storage/server.lua"); else lua_pop(L, 1);
        if (luaL_loadfile(L, SANDBOX_PATH("storage/replicated.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/storage/replicated.lua"); else lua_pop(L, 1);
    } else {
        lua_pushcfunction(L, luaopen_os); lua_pushstring(L, LUA_OSLIBNAME); lua_call(L, 1, 0);

        if (luaL_loadfile(L, SANDBOX_PATH("signal.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/signal.lua"); else lua_pop(L, 1);
        if (luaL_loadfile(L, SANDBOX_PATH("storage/local.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/storage/local.lua"); else lua_pop(L, 1);
        if (luaL_loadfile(L, SANDBOX_PATH("storage/replicated.lua")) == LUA_OK) lua_setfield(L, LUA_REGISTRYINDEX, "core/storage/replicated.lua"); else lua_pop(L, 1);

        lua_pushinteger(L, 0); lua_setfield(L, LUA_REGISTRYINDEX, "coroutine");
        lua_getglobal(L, "coroutine");
        lua_pushcfunction(L, coroutine_create); lua_setfield(L, -2, "create");
        lua_pushcfunction(L, coroutine_resume); lua_setfield(L, -2, "resume");
        lua_pop(L, 1);
    }

    return sandbox;
}

void sandbox_destroy(Sandbox *sandbox) {
    assert(sandbox != NULL);
    if (!sandbox) return;
    if (sandbox->state) lua_close(sandbox->state);
    allocator_destroy(sandbox->allocator);
    free(sandbox);
}

int sandbox_run(Sandbox *sandbox, const char *path) {
    assert(sandbox != NULL && path != NULL);
    if (!sandbox || !path) return 0;

    lua_State *L = sandbox->state;
    if (!L) return 0;

    lua_State *thread = lua_newthread(L);
    assert(thread != NULL);
    if (!thread) return 0;

    if (sandbox->context == SANDBOX_CLIENT)
        luaJIT_setmode(thread, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);

    lua_sethook(thread, governor, LUA_MASKCOUNT, SANDBOX_INSTRUCTIONS);

    if (sandbox->context == SANDBOX_SERVER) {
        if (luaL_loadfile(thread, SANDBOX_PATH("server.lua")) != LUA_OK) { fprintf(stderr, "sandbox: %s\n", lua_tostring(thread, -1)); lua_pop(L, 1); return 0; }
    } else {
        if (luaL_loadfile(thread, SANDBOX_PATH("client.lua")) != LUA_OK) { fprintf(stderr, "sandbox: %s\n", lua_tostring(thread, -1)); lua_pop(L, 1); return 0; }
    }

    if (lua_pcall(thread, 0, 1, 0) != LUA_OK) { fprintf(stderr, "sandbox: %s\n", lua_tostring(thread, -1)); lua_pop(L, 1); return 0; }
    if (lua_pcall(thread, 0, 1, 0) != LUA_OK) { fprintf(stderr, "sandbox: %s\n", lua_tostring(thread, -1)); lua_pop(L, 1); return 0; }
    if (luaL_loadfile(thread, path) != LUA_OK) { fprintf(stderr, "sandbox: %s\n", lua_tostring(thread, -1)); lua_pop(L, 1); return 0; }

    lua_pushvalue(thread, -2);
    lua_setfenv(thread, -2);
    lua_remove(thread, -2);

    int result = lua_resume(thread, 0);
    if (result != LUA_OK && result != LUA_YIELD) {
        fprintf(stderr, "sandbox: %s: %s\n", path, lua_tostring(thread, -1));
        lua_pop(L, 1);
        return 0;
    }

    lua_pop(L, 1);
    return 1;
}