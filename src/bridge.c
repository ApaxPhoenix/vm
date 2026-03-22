#include "bridge.h"
#include <stdio.h>

static int bridge_print(lua_State *state) {
    fprintf(stdout, "%s: print called\n", __FILE__);
    const int args = lua_gettop(state);
    for (int i = 1; i <= args; i++) {
        fprintf(stdout, "%s", lua_tostring(state, i));
        if (i < args) fprintf(stdout, "\t");
    }
    fprintf(stdout, "\n");
    return 0;
}

void bridge_register(Sandbox *sandbox) {
    fprintf(stdout, "%s: registering bridge functions\n", __FILE__);
    lua_register(sandbox->state, "print", bridge_print);
    fprintf(stdout, "%s: bridge ready\n", __FILE__);
}