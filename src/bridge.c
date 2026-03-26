#include "bridge.h"
#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>

static int bridge_print(lua_State *state) {
    const int args = lua_gettop(state);
    for (int i = 1; i <= args; i++) {
        if (lua_isboolean(state, i)) {
            fprintf(stdout, "%s", lua_toboolean(state, i) ? "true" : "false");
        } else if (lua_isnil(state, i)) {
            fprintf(stdout, "nil");
        } else if (luaL_getmetafield(state, i, "__tostring")) {
            lua_pushvalue(state, i);
            lua_call(state, 1, 1);
            fprintf(stdout, "%s", lua_tostring(state, -1));
            lua_pop(state, 1);
        } else if (lua_isnumber(state, i)) {
            fprintf(stdout, "%g", lua_tonumber(state, i));
        } else if (lua_isstring(state, i)) {
            fprintf(stdout, "%s", lua_tostring(state, i));
        } else {
            fprintf(stdout, "%s: %p", lua_typename(state, lua_type(state, i)), lua_topointer(state, i));
        }
        if (i < args) fprintf(stdout, "\t");
    }
    fprintf(stdout, "\n");
    return 0;
}

__attribute__((weak)) void bridge_register(const Sandbox *sandbox) {
    fprintf(stdout, "%s: registering bridge functions\n", __FILE__);
    lua_register(sandbox->state, "print", bridge_print);
    fprintf(stdout, "%s: bridge ready\n", __FILE__);
}
