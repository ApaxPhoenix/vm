#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

static void restrict_fds(void) {
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    fprintf(stdout, "%s: file descriptor limit: soft=%llu hard=%llu\n", __FILE__, rl.rlim_cur, rl.rlim_max);
    rl.rlim_cur = 8;
    rl.rlim_max = 8;
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        fprintf(stderr, "%s: warning: could not restrict file descriptors\n", __FILE__);
        return;
    }
    fprintf(stdout, "%s: file descriptors restricted to 8\n", __FILE__);
}

static void restrict_procs(void) {
    struct rlimit rl;
    getrlimit(RLIMIT_NPROC, &rl);
    fprintf(stdout, "%s: process limit: soft=%llu hard=%llu\n", __FILE__, rl.rlim_cur, rl.rlim_max);
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    if (setrlimit(RLIMIT_NPROC, &rl) != 0) {
        fprintf(stderr, "%s: warning: could not restrict process creation\n", __FILE__);
        return;
    }
    fprintf(stdout, "%s: process creation restricted\n", __FILE__);
}

static void restrict_core(void) {
    struct rlimit rl;
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        fprintf(stderr, "%s: warning: could not restrict core dumps\n", __FILE__);
        return;
    }
    fprintf(stdout, "%s: core dumps disabled\n", __FILE__);
}

static void restrict_stack(void) {
    struct rlimit rl;
    getrlimit(RLIMIT_STACK, &rl);
    fprintf(stdout, "%s: stack limit: soft=%llu hard=%llu\n", __FILE__, rl.rlim_cur, rl.rlim_max);
    rl.rlim_cur = 8 * 1024 * 1024;
    rl.rlim_max = 8 * 1024 * 1024;
    if (setrlimit(RLIMIT_STACK, &rl) != 0) {
        fprintf(stderr, "%s: warning: could not restrict stack size\n", __FILE__);
        return;
    }
    fprintf(stdout, "%s: stack restricted to 8MB\n", __FILE__);
}

void platform_sandbox(void) {
    fprintf(stdout, "%s: initializing sandbox on pid %d\n", __FILE__, getpid());
    restrict_fds();
    restrict_procs();
    restrict_core();
    restrict_stack();
    fprintf(stdout, "%s: sandbox active\n", __FILE__);
}