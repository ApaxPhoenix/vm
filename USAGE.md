# vm — Usage

## Overview

`vm` is a sandboxed Lua scripting library built on LuaJIT. It supports two execution contexts — **server** and **client** — each with different permissions and resource limits.

## Building

```bash
cmake -B build
cmake --build build
```

Dependencies (LuaJIT, libuv, enet, libwebsockets) are cloned and built automatically into `vendor/`.

Two CMake definitions are injected at build time:

| Define | Description |
|---|---|
| `VM_PATH` | Path to the `core/` Lua runtime files |
| `SCRIPTS_PATH` | Path to user scripts (vmtest only) |

## API

### Create a sandbox

```c
Sandbox *sandbox_create(Context context);
```

`context` is either `SANDBOX_SERVER` or `SANDBOX_CLIENT`.

- **Server** — gets `io`, `os`, `package`, full file access
- **Client** — gets `os` only, JIT disabled, coroutine limits enforced

### Run a script

```c
int sandbox_run(Sandbox *sandbox, const char *path);
// returns 1 on success, 0 on failure
```

The script runs inside the appropriate core wrapper (`core/server.lua` or `core/client.lua`) which sets up the environment before your script executes.

### Destroy a sandbox

```c
void sandbox_destroy(Sandbox *sandbox);
```

## Example

```c
Sandbox *server = sandbox_create(SANDBOX_SERVER);
Sandbox *client = sandbox_create(SANDBOX_CLIENT);

sandbox_run(server, SCRIPTS_PATH "/server.lua");
sandbox_run(client, SCRIPTS_PATH "/client.lua");

sandbox_destroy(server);
sandbox_destroy(client);
```

## Limits

| Limit | Value |
|---|---|
| Memory per sandbox | 64 MB |
| Instructions per tick | 1,000,000 |
| Max coroutines (client) | 16 |
| Max string size | 1 MB |
| Open files (unix) | 8 |
| Child processes (unix) | 0 |