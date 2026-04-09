#include "sandbox.h"
#include <stdlib.h>
#include <stdio.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luajit.h>
#include <assert.h>

#if defined(_WIN32)
#include <windows.h>
static void restrict_resources(void) {
    HANDLE job = CreateJobObject(nullptr, nullptr);
    assert(job != nullptr && "failed to create job object");
    JOBOBJECT_BASIC_LIMIT_INFORMATION limits = {0};
    limits.LimitFlags         = JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    limits.ActiveProcessLimit = 1;
    assert(SetInformationJobObject(job, JobObjectBasicLimitInformation, &limits, sizeof(limits)) && "failed to set job limits");
    assert(AssignProcessToJobObject(job, GetCurrentProcess()) && "failed to assign process to job");
}
#elif defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
#include <sys/resource.h>
#include <unistd.h>
static void restrict_resources(void) {
    struct rlimit rl;
    rl = (struct rlimit){ 8, 8 };                     setrlimit(RLIMIT_NOFILE, &rl);
    rl = (struct rlimit){ 0, 0 };                     setrlimit(RLIMIT_NPROC,  &rl);
    rl = (struct rlimit){ 0, 0 };                     setrlimit(RLIMIT_CORE,   &rl);
    rl = (struct rlimit){ 8*1024*1024, 8*1024*1024 }; setrlimit(RLIMIT_STACK,  &rl);
}
#else
static void restrict_resources(void) {}
#endif

static void governor(lua_State *state, lua_Debug *debug) {
    (void) debug;
    luaL_error(state, "vm: cpu budget exceeded");
}

static int load_chunk(lua_State *thread, lua_State *state, const char *path, const char *label) {
    if (luaL_loadfile(thread, path) != LUA_OK) {
        fprintf(stderr, "vm: %s: %s\n", label, lua_tostring(thread, -1));
        lua_pop(state, 1);
        return 0;
    }
    return 1;
}

static int call_chunk(lua_State *thread, lua_State *state, const char *label) {
    if (lua_pcall(thread, 0, 1, 0) != LUA_OK) {
        fprintf(stderr, "vm: %s: %s\n", label, lua_tostring(thread, -1));
        lua_pop(state, 1);
        return 0;
    }
    return 1;
}

Sandbox *sandbox_create(Context context) {
    restrict_resources();

    Sandbox *sandbox = malloc(sizeof(Sandbox));
    assert(sandbox != nullptr && "failed to allocate sandbox");

    sandbox->context   = context;
    sandbox->allocator = allocator_create(MEMORY);
    assert(sandbox->allocator != nullptr && "failed to create allocator");

    sandbox->state = lua_newstate(allocator_func, sandbox->allocator);
    assert(sandbox->state != nullptr && "failed to create lua state");

    lua_State *state = sandbox->state;

    lua_pushcfunction(state, luaopen_base);   lua_pushstring(state, "");               lua_call(state, 1, 0);
    lua_pushcfunction(state, luaopen_table);  lua_pushstring(state, LUA_TABLIBNAME);   lua_call(state, 1, 0);
    lua_pushcfunction(state, luaopen_string); lua_pushstring(state, LUA_STRLIBNAME);   lua_call(state, 1, 0);
    lua_pushcfunction(state, luaopen_math);   lua_pushstring(state, LUA_MATHLIBNAME);  lua_call(state, 1, 0);
    lua_pushcfunction(state, luaopen_bit);    lua_pushstring(state, LUA_BITLIBNAME);   lua_call(state, 1, 0);

    if (context == SERVER) {
        lua_pushcfunction(state, luaopen_io);      lua_pushstring(state, LUA_IOLIBNAME);   lua_call(state, 1, 0);
        lua_pushcfunction(state, luaopen_os);      lua_pushstring(state, LUA_OSLIBNAME);   lua_call(state, 1, 0);
        lua_pushcfunction(state, luaopen_package); lua_pushstring(state, LUA_LOADLIBNAME); lua_call(state, 1, 0);
    }

    {
        const char *unsafe[] = {
            "debug", "collectgarbage", "dofile",
            "load", "loadfile", "loadstring",
            nullptr
        };
        for (int i = 0; unsafe[i] != nullptr; i++) {
            lua_pushnil(state); lua_setglobal(state, unsafe[i]);
        }
    }

    if (context == CLIENT) {
        const char *unsafe[] = { "io", "os", "require", "package", nullptr };
        for (int i = 0; unsafe[i] != nullptr; i++) {
            lua_pushnil(state); lua_setglobal(state, unsafe[i]);
        }
    }

    return sandbox;
}

void sandbox_destroy(Sandbox *sandbox) {
    assert(sandbox != nullptr && "sandbox is null");
    if (sandbox->state != nullptr) lua_close(sandbox->state);
    allocator_destroy(sandbox->allocator);
    free(sandbox);
}

int sandbox_run(Sandbox *sandbox, const char *path) {
    assert(sandbox != nullptr && "sandbox is null");
    assert(path    != nullptr && "path is null");

    lua_State *state  = sandbox->state;
    lua_State *thread = lua_newthread(state);
    assert(thread != nullptr && "failed to create lua thread");

    if (sandbox->context == CLIENT)
        luaJIT_setmode(thread, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);

    lua_sethook(thread, governor, LUA_MASKCOUNT, INSTRUCTIONS);

    const char *init = sandbox->context == SERVER
        ? "vendor/vm/server.lua"
        : "vendor/vm/client.lua";

    if (!load_chunk(thread, state, init,   "init"))   return 0;
    if (!call_chunk(thread, state,         "init"))   return 0;
    if (!call_chunk(thread, state,         "isolate")) return 0;
    if (!load_chunk(thread, state, path,   "script")) return 0;

    lua_pushvalue(thread, -2);
    lua_setfenv(thread, -2);
    lua_remove(thread, -2);

    int result = lua_resume(thread, 0);
    if (result != LUA_OK && result != LUA_YIELD) {
        fprintf(stderr, "vm: runtime: %s\n", lua_tostring(thread, -1));
        lua_pop(state, 1);
        return 0;
    }

    lua_pop(state, 1);
    return 1;
}