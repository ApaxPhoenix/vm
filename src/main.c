#include "sandbox.h"
#include "bridge.h"
#include "platform.h"
#include <stdio.h>

int main(void) {
    platform_sandbox();

    Sandbox *sandbox = sandbox_create();
    if (sandbox == NULL) {
        fprintf(stderr, "vm: failed to create sandbox\n");
        return 1;
    }

    bridge_register(sandbox);

    fprintf(stdout, "main.c: testing loadstring exploit\n");
    sandbox_run(sandbox, "loadstring('print(os.execute())')()");

    fprintf(stdout, "main.c: testing os.execute exploit\n");
    sandbox_run(sandbox, "os.execute('rm -rf /')");

    fprintf(stdout, "main.c: testing ffi exploit\n");
    sandbox_run(sandbox, "local ffi = require('ffi') ffi.C.system('ls')");

    fprintf(stdout, "main.c: testing require exploit\n");
    sandbox_run(sandbox, "require('os')");

    fprintf(stdout, "main.c: testing infinite loop exploit\n");
    sandbox_run(sandbox, "while true do end");

    fprintf(stdout, "main.c: testing memory exploit\n");
    sandbox_run(sandbox, "local t = {} for i = 1, 1000000 do t[i] = string.rep('x', 1000) end");

    fprintf(stdout, "main.c: all exploits tested, vm still running\n");
    sandbox_run(sandbox, "print('vm is running!')");

    sandbox_destroy(sandbox);
    return 0;
}